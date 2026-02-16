#include "nthframe.h"

#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <cmath>

constexpr auto DESCRIPTION_STYLE =
    "color: rgb(58, 58, 58); border-left: 6px solid rgb(58, 58, 58); "
    "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "
    "background-color: lightblue;";

constexpr auto SETTING_KEY_N = "N";
constexpr auto SETTING_KEY_KEEP_ISOLATED = "KeepIsolated";

NthFrame::NthFrame() : IBase() {
    // Install translator for localization support
    QLocale locale = qApp->property("translation").toLocale();
    auto translator = new QTranslator();
    translator->load(locale, "nth", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

QString NthFrame::getName() const {
    return tr("Nth image selection");
}

SettingsWidgetResult NthFrame::getSettingsWidget(QWidget* parent) {
    if (!m_settingsWidget) {
        createSettingsWidget(parent);
    }
    if (m_spinBox) {
        m_spinBox->setValue(static_cast<int>(m_n));
    }
    return m_settingsWidget;
}

QMap<QString, QVariant> NthFrame::getSettings() const {
    QMap<QString, QVariant> settings;
    settings.insert(SETTING_KEY_N, QString::number(m_n));
    settings.insert(SETTING_KEY_KEEP_ISOLATED, m_keepIsolated);
    return settings;
}

ApplySettingsResult NthFrame::applySettings(const QMap<QString, QVariant>& settings) {
    auto it = settings.find(SETTING_KEY_N);
    if (it != settings.end()) {
        bool ok = false;
        unsigned int value = it.value().toUInt(&ok);
        if (ok && value > 0) {
            m_n = value;
        } else {
            return tl::make_unexpected(Error(ErrorCode::InvalidInput,"Invalid value for setting N"));
        }
    }

    it = settings.find(SETTING_KEY_KEEP_ISOLATED);
    if (it != settings.end()) {
        m_keepIsolated = it.value().toBool();
    }

    // Update UI if widget exists
    if (m_spinBox) {
        m_spinBox->setValue(static_cast<int>(m_n));
    }
    if (m_checkBox) {
        m_checkBox->setChecked(m_keepIsolated);
    }

    return {};
}

void NthFrame::activate() {
    // Called when the plugin is activated in iVS3D
}

void NthFrame::deactivate() {
    // Called when the plugin is deactivated in iVS3D
}

void NthFrame::onCudaChanged(bool enabled) {
    // This plugin doesn't use CUDA, so no action needed
    (void)enabled;
}

InputLoadedResult NthFrame::onInputLoaded(const InputData& input) {
    m_numFrames = input.reader ? input.reader->getPicCount() : 0;
    if (input.reader->getFPS() > 0) {
        m_fps = round(input.reader->getFPS());
    } else {
        m_fps = 30; // Default to 30 FPS if not available
    }
    m_n = m_fps; // Default N to the FPS for roughly 1 second intervals
    if (m_spinBox) {
        m_spinBox->setValue(static_cast<int>(m_n));
    }
    return {};
}

SelectionResult NthFrame::selectImages(const SelectionData& data,
                                        volatile bool& cancelFlag) {
    const auto& imageList = data.selectedIndices;

    if (imageList.empty()) {
        return std::vector<uint>();
    }

    const int expectedKfDistance = roundf((float)m_n / ((float)imageList.size() / m_numFrames));
    qDebug() << "Expected Distance: " << expectedKfDistance;
    qDebug() << "N: " << m_n;

    std::vector<uint> keyframes;
    keyframes.reserve(1 + imageList.size() / m_n);

    uint lastKeyframeIndex = imageList.front();
    uint counter = UINT_MAX - 2;

    for (size_t i = 0; i < imageList.size(); ++i) {
        // Check for cancellation
        if (cancelFlag) {
            return tl::make_unexpected(Error(ErrorCode::RuntimeError,"Selection cancelled by user"));
        }

        const int progress = static_cast<int>((i * 100) / imageList.size());
        emit updateProgress(progress, tr("getting every n-th frame"));

        counter++;

        const uint currentIndex = imageList[i];
        const uint distanceFromLast = currentIndex - lastKeyframeIndex;

        // Select keyframe if:
        // 1. We've reached the Nth frame, OR
        // 2. We keep isolated images and this frame is significantly far from the last keyframe
        if (counter >= m_n ||
            (m_keepIsolated && distanceFromLast > static_cast<uint>(expectedKfDistance))) {
            keyframes.push_back(currentIndex);
            lastKeyframeIndex = currentIndex;
            counter = 0;
        }
    }

    emit updateProgress(100, tr("Nth-Image progress"));

    keyframes.shrink_to_fit();
    return keyframes;
}

void NthFrame::slot_nChanged(int n) {
    m_n = static_cast<unsigned int>(std::max(1, n));
}

void NthFrame::slot_checkboxToggled(bool checked) {
    m_keepIsolated = checked;
}

void NthFrame::createSettingsWidget(QWidget* parent) {
    m_settingsWidget = std::make_shared<QWidget>(parent);
    auto* layout = new QVBoxLayout(m_settingsWidget.get());
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create N value selector
    auto* nSelectorWidget = new QWidget(parent);
    auto* nSelectorLayout = new QHBoxLayout(nSelectorWidget);
    nSelectorLayout->setSpacing(0);
    nSelectorLayout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(tr("Select every Nth image"), parent);
    nSelectorLayout->addWidget(label);

    m_spinBox = new QSpinBox(parent);
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(9999);
    m_spinBox->setValue(static_cast<int>(m_fps));
    m_spinBox->setAlignment(Qt::AlignRight);
    nSelectorLayout->addWidget(m_spinBox);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &NthFrame::slot_nChanged);

    layout->addWidget(nSelectorWidget);

    // Create "keep isolated" checkbox
    m_checkBox = new QCheckBox(tr("Keep isolated images"), parent);
    m_checkBox->setChecked(m_keepIsolated);
    connect(m_checkBox, &QCheckBox::toggled, this, &NthFrame::slot_checkboxToggled);
    layout->addWidget(m_checkBox);

    // Add description
    auto* descriptionLabel = new QLabel(
        tr("When selecting strictly every Nth frame, isolated images or "
           "small batches are unlikely to get selected for larger N."),
        parent);
    descriptionLabel->setStyleSheet(DESCRIPTION_STYLE);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}

