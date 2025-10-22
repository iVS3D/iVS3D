#include "blur.h"

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
    QTranslator *translator = new QTranslator();
    translator->load(locale, "blur", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
    m_settingsWidget = nullptr;
    m_blurAlgorithms.push_back(new BlurLaplacian());
    m_blurAlgorithms.push_back(new BlurSobel());
    m_blurAlgorithms.push_back(new BlurTenengrad());
    m_usedBlur = m_blurAlgorithms[0];

    m_localDeviation = 95;  // keep >= 95% of local median
    m_windowSize = 30;      // images on each side for the local window
}

QWidget *Blur::getSettingsWidget(QWidget *parent) {
    if (!m_settingsWidget) {
        createSettingsWidget(parent);
    }
    return m_settingsWidget;
}

std::vector<uint> Blur::sampleImages(const std::vector<unsigned int> &imageList,
                                     Progressable *receiver,
                                     volatile bool *stopped, bool useCuda,
                                     LogFileParent *logFile) {
    g_useCuda = useCuda;

    m_logFile = logFile;
    m_logFile->startTimer("complete");
    m_blurValues.clear();

    if (m_buffer.size() != 0) {
        QMapIterator<QString, QVariant> mapIt(m_buffer);
        while (mapIt.hasNext()) {
            mapIt.next();
            if (mapIt.key().compare(m_usedBlur->getName()) == 0) {
                m_blurValues = splitDoubleString(mapIt.value().toString());
                break;
            }
        }
    }
    if (m_blurValues.empty()) {
        m_blurValues = std::vector<double>(m_reader->getPicCount());
    }

    std::vector<uint> sampledImages =
        sampleKeyframes(m_reader, receiver, stopped, imageList);

    m_logFile->stopTimer();
    computeBuffer();
    return sampledImages;
}

QString Blur::getName() const { return tr("Blur detection"); }

void Blur::computeBuffer() {
    std::stringstream bufferStream;
    for (uint i = 0; i < m_blurValues.size(); i++) {
        if (i != 0) bufferStream << ",";
        bufferStream << m_blurValues[i];
    }
    std::string buffer = bufferStream.str();
    QVariant blurValues(QString::fromStdString(buffer));
    m_buffer.insert(m_usedBlur->getName(), blurValues);
    emit updateBuffer(m_buffer);
}

void Blur::initialize(Reader *reader, QMap<QString, QVariant> buffer,
                      signalObject *sig_obj) {
    m_reader = reader;
    m_buffer = buffer;
    m_sigObj = sig_obj;
    if (m_settingsWidget) {
        if (m_buffer.contains(m_usedBlur->getName())) {
            QVariant currentBuffer = m_buffer[m_usedBlur->getName()];
            double currentBlurValue =
                splitDoubleString(currentBuffer.toString())[0];
            QString info = currentBlurValue == 0
                               ? tr("not calculated")
                               : QString::number(currentBlurValue);
            m_infoLabel->setText(tr("Blur value for the current image is ") +
                                 info);
        } else {
            m_infoLabel->setText(
                tr("Blur value for the current image is not calculated"));
        }
    }
    if (m_sigObj)
        connect(m_sigObj, SIGNAL(sig_selectedImageIndex(uint)), this,
                SLOT(slot_selectedImageIndex(uint)));
}

void Blur::setSettings(QMap<QString, QVariant> settings) {
    m_windowSize = settings.find(WINDOW_SIZE).value().toInt();
    m_localDeviation = settings.find(LOCAL_DEVIATION).value().toDouble();
    QString usedAlgo = settings.find(USED_BLUR).value().toString();
    int blurIndex = 0;
    for (BlurAlgorithm *algo : m_blurAlgorithms) {
        if (usedAlgo.compare(algo->getName()) == 0) {
            m_usedBlur = algo;
            break;
        }
        blurIndex++;
    }
    if (m_settingsWidget) {
        m_spinBoxLD->setValue(static_cast<int>(m_localDeviation));
        m_spinBoxWS->setValue(m_windowSize);
        m_comboBoxBlur->setCurrentIndex(blurIndex);
    }
}

QMap<QString, QVariant> Blur::generateSettings(Progressable *receiver,
                                               bool useCuda,
                                               volatile bool *stopped) {
    (void)receiver;
    (void)useCuda;
    (void)stopped;
    return getSettings();
}

QMap<QString, QVariant> Blur::getSettings() {
    QMap<QString, QVariant> settings;
    settings.insert(USED_BLUR, m_usedBlur->getName());
    settings.insert(LOCAL_DEVIATION, m_localDeviation);
    settings.insert(WINDOW_SIZE, m_windowSize);
    return settings;
}

void Blur::slot_blurAlgoChanged(const QString &name) {
    for (BlurAlgorithm *b : m_blurAlgorithms) {
        if (b->getName().compare(name) == 0) {
            m_usedBlur = b;
            break;
        }
    }
}

void Blur::slot_wsChanged(int ws) { m_windowSize = ws; }

void Blur::slot_ldChanged(int ld) { m_localDeviation = ld; }

void Blur::slot_selectedImageIndex(uint index) {
    if (m_buffer.contains(m_usedBlur->getName())) {
        QVariant currentBuffer = m_buffer[m_usedBlur->getName()];
        double currentBlurValue =
            splitDoubleString(currentBuffer.toString())[index];
        QString info = currentBlurValue == 0
                           ? tr("not calculated")
                           : QString::number(currentBlurValue);
        if (m_settingsWidget) {
            m_infoLabel->setText(tr("Blur value for the current image is ") +
                                 info);
        }
    }
}

void Blur::createSettingsWidget(QWidget *parent) {
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout());
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);

    QWidget *w = new QWidget(parent);
    w->setLayout(new QHBoxLayout(parent));
    w->layout()->setSpacing(0);
    w->layout()->setMargin(0);
    w->layout()->addWidget(new QLabel(tr("Select filter "), parent));

    m_comboBoxBlur = new QComboBox(parent);
    for (BlurAlgorithm *b : m_blurAlgorithms) {
        m_comboBoxBlur->addItem(b->getName());
    }
    w->layout()->addWidget(m_comboBoxBlur);
    QObject::connect(
        m_comboBoxBlur,
        QOverload<const QString &>::of(&QComboBox::currentTextChanged), this,
        &Blur::slot_blurAlgoChanged);

    m_settingsWidget->layout()->addWidget(w);

    QLabel *LabelBlur = new QLabel(tr("Blur algorithm to be used"));
    LabelBlur->setStyleSheet(DESCRIPTION_STYLE);
    LabelBlur->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(LabelBlur);

    QWidget *ws = new QWidget(parent);
    ws->setLayout(new QHBoxLayout(parent));
    ws->layout()->setSpacing(0);
    ws->layout()->setMargin(0);
    ws->layout()->addWidget(new QLabel(tr("Set window size"), parent));

    m_spinBoxWS = new QSpinBox(parent);
    m_spinBoxWS->setMinimum(1);
    m_spinBoxWS->setMaximum(9999);
    m_spinBoxWS->setValue(m_windowSize);
    m_spinBoxWS->setAlignment(Qt::AlignRight);
    ws->layout()->addWidget(m_spinBoxWS);
    QObject::connect(m_spinBoxWS, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &Blur::slot_wsChanged);

    m_settingsWidget->layout()->addWidget(ws);

    QLabel *windowSize = new QLabel(
        tr("Number of images used on each side for the local window"));
    windowSize->setStyleSheet(DESCRIPTION_STYLE);
    windowSize->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(windowSize);

    QWidget *ld = new QWidget(parent);
    ld->setLayout(new QHBoxLayout(parent));
    ld->layout()->setSpacing(0);
    ld->layout()->setMargin(0);
    ld->layout()->addWidget(new QLabel(tr("Local threshold (%)"), parent));

    m_spinBoxLD = new QSpinBox(parent);
    m_spinBoxLD->setMinimum(1);
    m_spinBoxLD->setMaximum(200);
    m_spinBoxLD->setValue(static_cast<int>(m_localDeviation));
    m_spinBoxLD->setAlignment(Qt::AlignRight);
    ld->layout()->addWidget(m_spinBoxLD);
    QObject::connect(m_spinBoxLD, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &Blur::slot_ldChanged);

    m_settingsWidget->layout()->addWidget(ld);

    QLabel *localDeviation =
        new QLabel(tr("Keep images whose sharpness is at least this percent of "
                      "the local median."));
    localDeviation->setStyleSheet(DESCRIPTION_STYLE);
    localDeviation->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(localDeviation);

    m_infoLabel =
        new QLabel(tr("Blur value for the current image is not calculated"));
    m_infoLabel->setWordWrap(true);
    m_settingsWidget->layout()->addWidget(m_infoLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

std::vector<double> Blur::splitDoubleString(QString string) {
    std::vector<double> returnVector;
    QStringList values = string.split(",");
    for (const QString &val : qAsConst(values)) {
        if (!val.isEmpty()) {
            returnVector.push_back(val.toDouble());
        }
    }
    return returnVector;
}

// Percent-of-median local threshold for provided indices
std::vector<uint> Blur::sampleKeyframes(Reader *reader, Progressable *receiver,
                                        volatile bool *stopped,
                                        std::vector<uint> indices) {
    std::vector<uint> sampledImages;
    int picCount = static_cast<int>(indices.size());

    for (int i = 0; i < picCount; i++) {
        if (*stopped) return {};

        uint idx = indices[i];

        if (receiver != nullptr) {
            int progress = (i * 100 / picCount);
            QString currentProgress = progressMessage(i, picCount);
            QMetaObject::invokeMethod(
                receiver, "slot_makeProgress", Qt::DirectConnection,
                Q_ARG(int, progress), Q_ARG(QString, currentProgress));
        }

        int ws = m_windowSize;
        int wStart = (idx > (uint)ws) ? int(idx) - ws : 0;
        int wEnd = std::min<int>(int(idx) + ws, m_reader->getPicCount() - 1);

        std::vector<double> windowVals;
        windowVals.reserve(wEnd - wStart + 1);
        for (int w = wStart; w <= wEnd; ++w) {
            if (m_blurValues[w] == 0) {
                m_blurValues[w] = m_usedBlur->calcOneBluriness(reader, w);
            }
            windowVals.push_back(m_blurValues[w]);
        }

        if (m_blurValues[idx] == 0) {
            m_blurValues[idx] = m_usedBlur->calcOneBluriness(reader, idx);
        }

        double med = medianOf(windowVals);
        if (med <= 1e-9) {
            sampledImages.push_back(idx);
            continue;
        }

        double ratio = m_localDeviation / 100.0;
        if (m_blurValues[idx] >= med * ratio) {
            sampledImages.push_back(idx);
        }
    }

    QMetaObject::invokeMethod(receiver, "slot_makeProgress",
                              Qt::DirectConnection, Q_ARG(int, 100),
                              Q_ARG(QString, tr("Blur progress")));
    return sampledImages;
}

QString Blur::progressMessage(int curr, int total) {
    return tr("Calculate blur value for image ") + QString::number(curr) +
           tr(" of ") + QString::number(total);
}
