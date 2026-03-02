#include "blur.h"

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QVBoxLayout>

// Global CUDA switch shared with algorithms
extern bool g_useCuda;

// Helper to compute median
static double medianOf(std::vector<double> v) {
    if (v.empty()) return 0.0;
    size_t n = v.size();
    size_t mid = n / 2;
    std::nth_element(v.begin(), v.begin() + mid, v.end());
    double med = v[mid];
    if ((n % 2) == 0) {
        std::nth_element(v.begin(), v.begin() + mid - 1, v.end());
        med = 0.5 * (med + v[mid - 1]);
    }
    return med;
}

Blur::Blur() {
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "blur", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);

    m_blurAlgorithms.push_back(new BlurLaplacian());
    m_blurAlgorithms.push_back(new BlurSobel());
    m_blurAlgorithms.push_back(new BlurTenengrad());
    m_usedBlur = m_blurAlgorithms[0];
}

Blur::~Blur() {
    for (BlurAlgorithm* algo : m_blurAlgorithms) {
        delete algo;
    }
    m_blurAlgorithms.clear();
}

SettingsWidgetResult Blur::getSettingsWidget() {
    return createSettingsWidget();
}

QString Blur::getName() const { return tr("Blur detection"); }

QMap<QString, QVariant> Blur::getSettings() const {
    QMap<QString, QVariant> settings;
    settings.insert(USED_BLUR, m_usedBlur->getName());
    settings.insert(LOCAL_DEVIATION, m_localDeviation);
    settings.insert(WINDOW_SIZE, m_windowSize);
    return settings;
}

ApplySettingsResult Blur::applySettings(
    const QMap<QString, QVariant>& settings) {
    int windowSize = settings.value(WINDOW_SIZE, m_windowSize).toInt();
    double localDeviation =
        settings.value(LOCAL_DEVIATION, m_localDeviation).toDouble();
    QString usedAlgo =
        settings.value(USED_BLUR, m_usedBlur->getName()).toString();

    // Backward compatibility: old projects used 1..200 (%) with meaningful
    // values typically near 100. New UI uses an inverted 0.0..10.0 scale.
    if (localDeviation > 10.0) {
        localDeviation = std::clamp(100.0 - localDeviation, 0.0, 10.0);
    }

    if (windowSize < 1 || localDeviation < 0.0 || localDeviation > 10.0) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("Invalid blur settings")));
    }

    bool algoChanged = (usedAlgo != m_usedBlur->getName());
    for (BlurAlgorithm* algo : m_blurAlgorithms) {
        if (usedAlgo == algo->getName()) {
            m_usedBlur = algo;
            break;
        }
    }

    m_windowSize = windowSize;
    m_localDeviation = localDeviation;

    if (algoChanged) {
        invalidateCache();
    }

    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    emit updatePreview(true);
    return {};
}

void Blur::activate() {}

void Blur::deactivate() {}

void Blur::onCudaChanged(bool enabled) {
    m_useCuda = enabled;
    g_useCuda = enabled;
    invalidateCache();
    emit updatePreview(true);
}

InputLoadedResult Blur::onInputLoaded(const InputData& input) {
    if (!input.reader) {
        return {};
    }

    m_reader = input.reader;
    m_currentIndex = 0;
    invalidateCache();
    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    return {};
}

void Blur::onIndexChanged(uint index) {
    m_currentIndex = index;
    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
}

VisualizationResult Blur::generatePreview(const PreviewData& data) {
    cv::Mat debugImage;
    m_usedBlur->calcOneBluriness(data.image, &debugImage);
    
    // Process the debug image to create a visualization overlay
    // 1. find minimum and maximum value for normalization
    // 2. convert to heatmap
    // 3. scale to 0..255 and convert to 8-bit
    double minVal = 0.0, maxVal = 0.0;
    cv::minMaxLoc(debugImage, &minVal, &maxVal);
    if (maxVal > 0.0) {
        double range = maxVal - minVal;
        if (range > 0.0) {
            debugImage = (debugImage - minVal) / range;  // normalize to 0..1
        }
        debugImage.convertTo(debugImage, CV_8U, 255.0);  // scale to 0..255
        cv::applyColorMap(debugImage, debugImage, cv::COLORMAP_INFERNO);
    } else {
        debugImage.setTo(cv::Scalar(0, 0, 255));  // red for invalid frames
    }

    cv::Mat colormap(cv::Size(20, data.image.rows-20), CV_8U, cv::Scalar(0));
    // visualize the INFERNO colormap as a legend on the left side of the preview
    for (int y = 0; y < colormap.rows; ++y) {
        double value = 1.0 - static_cast<double>(y) / colormap.rows;  // invert for better visibility
        uint8_t colorValue = static_cast<uint8_t>(value * 255.0);
        colormap.row(y).setTo(colorValue);
    }
    cv::applyColorMap(colormap, colormap, cv::COLORMAP_INFERNO);

    Visualization vis;
    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Source Image");
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::FullImage;
    }
    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Edge Detection (min: %1, max: %2)").arg(minVal, 0, 'f', 2).arg(maxVal, 0, 'f', 2);
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::RegionOfInterest;
        ImageOverlay overlay;
        overlay.image = std::move(debugImage);
        view.overlays.push_back(overlay);
    }

    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Legend");
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::FullImage;
        view.style.relativeSize = QPointF(0.2, 1.0);

        ImageOverlay overlay;
        overlay.image = colormap;
        overlay.style.position = QRectF(0.05, 0.15, 0.2, 0.8); // position on the left with some padding
        view.overlays.push_back(overlay);

        TextOverlay textOverlayMax;
        textOverlayMax.text = tr("Edge (%1)").arg(maxVal, 0, 'f', 2);
        textOverlayMax.position = QPointF(0.3, 0.15);
        textOverlayMax.anchor = TextAnchor::TopLeft;
        view.overlays.push_back(textOverlayMax);
        

        TextOverlay textOverlayMin;
        textOverlayMin.text = tr("Smooth (%1)").arg(minVal, 0, 'f', 2);
        textOverlayMin.position = QPointF(0.3, 0.95);
        textOverlayMin.anchor = TextAnchor::BottomLeft;
        view.overlays.push_back(textOverlayMin);
    }
    return vis;
}

SelectionResult Blur::selectImages(const SelectionData& data,
                                   volatile bool& cancelFlag) {
    g_useCuda = m_useCuda;

    Reader* reader = data.reader ? data.reader : m_reader;
    if (!reader) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("No reader available")));
    }

    const bool inputChanged = (m_cachedPicCount != reader->getPicCount()) ||
                              (m_cachedAlgoName != m_usedBlur->getName());
    if (inputChanged) {
        m_cachedPicCount = reader->getPicCount();
        m_cachedAlgoName = m_usedBlur->getName();
        m_cachedBlurValues.assign(m_cachedPicCount, 0.0);
    }

    auto sampledImages =
        sampleKeyframes(reader, cancelFlag, data.selectedIndices);

    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    return sampledImages;
}

void Blur::slot_blurAlgoChanged(const QString& name) {
    if (m_usedBlur && m_usedBlur->getName() == name) {
        return;
    }
    for (BlurAlgorithm* b : m_blurAlgorithms) {
        if (b->getName() == name) {
            m_usedBlur = b;
            break;
        }
    }
    invalidateCache();
    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    emit updatePreview(true);
}

void Blur::slot_wsChanged(int ws) {
    m_windowSize = std::max(1, ws);
    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    emit updatePreview(true);
}

void Blur::slot_ldChanged(double ld) {
    m_localDeviation = std::clamp(ld, 0.0, 10.0);
    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    emit updatePreview(true);
}

std::unique_ptr<QWidget> Blur::createSettingsWidget() {
    auto settingsWidget = std::make_unique<QWidget>(nullptr);
    settingsWidget->setLayout(new QVBoxLayout());
    settingsWidget->layout()->setSpacing(0);
    settingsWidget->layout()->setContentsMargins(0, 0, 0, 0);

    QWidget* w = new QWidget(settingsWidget.get());
    w->setLayout(new QHBoxLayout());
    w->layout()->setSpacing(0);
    w->layout()->setContentsMargins(0, 0, 0, 0);
    w->layout()->addWidget(
        new QLabel(tr("Select filter "), settingsWidget.get()));

    m_comboBoxBlur = new QComboBox(settingsWidget.get());
    for (BlurAlgorithm* b : m_blurAlgorithms) {
        m_comboBoxBlur->addItem(b->getName());
    }
    w->layout()->addWidget(m_comboBoxBlur);
    connect(m_comboBoxBlur,
            QOverload<const QString&>::of(&QComboBox::currentTextChanged), this,
            &Blur::slot_blurAlgoChanged);
    settingsWidget->layout()->addWidget(w);

    QLabel* labelBlur =
        new QLabel(tr("Blur algorithm to be used"), settingsWidget.get());
    labelBlur->setStyleSheet(DESCRIPTION_STYLE);
    labelBlur->setWordWrap(true);
    settingsWidget->layout()->addWidget(labelBlur);

    QWidget* ws = new QWidget(settingsWidget.get());
    ws->setLayout(new QHBoxLayout());
    ws->layout()->setSpacing(0);
    ws->layout()->setContentsMargins(0, 0, 0, 0);
    ws->layout()->addWidget(
        new QLabel(tr("Set window size"), settingsWidget.get()));

    m_spinBoxWS = new QSpinBox(settingsWidget.get());
    m_spinBoxWS->setMinimum(1);
    m_spinBoxWS->setMaximum(9999);
    m_spinBoxWS->setAlignment(Qt::AlignRight);
    ws->layout()->addWidget(m_spinBoxWS);
    connect(m_spinBoxWS, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Blur::slot_wsChanged);
    settingsWidget->layout()->addWidget(ws);

    QLabel* windowSize = new QLabel(
        tr("Number of images used on each side for the local window"),
        settingsWidget.get());
    windowSize->setStyleSheet(DESCRIPTION_STYLE);
    windowSize->setWordWrap(true);
    settingsWidget->layout()->addWidget(windowSize);

    QWidget* ld = new QWidget(settingsWidget.get());
    ld->setLayout(new QHBoxLayout());
    ld->layout()->setSpacing(0);
    ld->layout()->setContentsMargins(0, 0, 0, 0);
    ld->layout()->addWidget(
        new QLabel(tr("Sharpness tolerance (%)"), settingsWidget.get()));

    m_spinBoxLD = new QDoubleSpinBox(settingsWidget.get());
    m_spinBoxLD->setMinimum(0.0);
    m_spinBoxLD->setMaximum(10.0);
    m_spinBoxLD->setSingleStep(0.1);
    m_spinBoxLD->setDecimals(1);
    m_spinBoxLD->setAlignment(Qt::AlignRight);
    ld->layout()->addWidget(m_spinBoxLD);
    connect(m_spinBoxLD, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &Blur::slot_ldChanged);
    settingsWidget->layout()->addWidget(ld);

    QLabel* localDeviation =
        new QLabel(tr("Sharpness tolerance relative to the local median "
                      "(0.0 = strict, 10.0 = permissive)."),
                   settingsWidget.get());
    localDeviation->setStyleSheet(DESCRIPTION_STYLE);
    localDeviation->setWordWrap(true);
    settingsWidget->layout()->addWidget(localDeviation);

    m_infoLabel =
        new QLabel(tr("Blur value for the current image is not calculated"),
                   settingsWidget.get());
    m_infoLabel->setWordWrap(true);
    settingsWidget->layout()->addWidget(m_infoLabel);

    connect(
        this, &Blur::syncSettingsWidget, settingsWidget.get(),
        [this](const QString& algorithmName, int windowSize, double localDeviation,
               const QString& infoText) {
            if (!m_comboBoxBlur || !m_spinBoxWS || !m_spinBoxLD ||
                !m_infoLabel) {
                return;
            }
            const QSignalBlocker b1(m_comboBoxBlur);
            const QSignalBlocker b2(m_spinBoxWS);
            const QSignalBlocker b3(m_spinBoxLD);

            int idx = m_comboBoxBlur->findText(algorithmName);
            if (idx >= 0) {
                m_comboBoxBlur->setCurrentIndex(idx);
            }
            m_spinBoxWS->setValue(windowSize);
            m_spinBoxLD->setValue(std::clamp(localDeviation, 0.0, 10.0));
            m_infoLabel->setText(infoText);
        },
        Qt::QueuedConnection);

    settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    settingsWidget->adjustSize();

    emit syncSettingsWidget(m_usedBlur->getName(), m_windowSize,
                            m_localDeviation, currentInfoText());
    return settingsWidget;
}

QString Blur::currentInfoText() const {
    if (!m_reader || m_cachedBlurValues.empty() ||
        m_currentIndex >= m_cachedBlurValues.size()) {
        return tr("Blur value for the current image is not calculated");
    }

    double currentBlurValue = m_cachedBlurValues[m_currentIndex];
    QString info = currentBlurValue == 0.0 ? tr("not calculated")
                                           : QString::number(currentBlurValue);
    return tr("Blur value for the current image is ") + info;
}

std::vector<uint> Blur::sampleKeyframes(Reader* reader,
                                        volatile bool& cancelFlag,
                                        const std::vector<uint>& indices) {
    std::vector<uint> sampledImages;
    int picCount = static_cast<int>(indices.size());

    if (picCount == 0) {
        emit updateProgress(100, tr("Blur progress"));
        return sampledImages;
    }

    for (int i = 0; i < picCount; i++) {
        if (cancelFlag) {
            return sampledImages;
        }

        const uint idx = indices[i];
        emit updateProgress((i * 100) / picCount, progressMessage(i, picCount));

        int ws = m_windowSize;
        int wStart = (idx > static_cast<uint>(ws)) ? int(idx) - ws : 0;
        int wEnd = std::min<int>(int(idx) + ws, reader->getPicCount() - 1);

        std::vector<double> windowVals;
        windowVals.reserve(wEnd - wStart + 1);
        for (int w = wStart; w <= wEnd; ++w) {
            if (m_cachedBlurValues[w] == 0.0) {
                m_cachedBlurValues[w] = m_usedBlur->calcOneBluriness(reader->getPic(w), nullptr);
            }
            windowVals.push_back(m_cachedBlurValues[w]);
        }

        if (m_cachedBlurValues[idx] == 0.0) {
            m_cachedBlurValues[idx] = m_usedBlur->calcOneBluriness(reader->getPic(idx), nullptr);
        }

        double med = medianOf(windowVals);
        if (med <= 1e-9) {
            sampledImages.push_back(idx);
            continue;
        }

        const double ratio = 1.0 - (m_localDeviation / 100.0);
        if (m_cachedBlurValues[idx] >= med * ratio) {
            sampledImages.push_back(idx);
        }
    }

    emit updateProgress(100, tr("Blur progress"));
    return sampledImages;
}

QString Blur::progressMessage(int curr, int total) const {
    return tr("Calculate blur value for image ") + QString::number(curr) +
           tr(" of ") + QString::number(total);
}

void Blur::invalidateCache() {
    m_cachedBlurValues.clear();
    m_cachedPicCount = 0;
    m_cachedAlgoName.clear();
}
