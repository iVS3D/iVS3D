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
#include <QSignalBlocker>
#include <cmath>

constexpr auto DESCRIPTION_STYLE =
    "color: rgb(58, 58, 58); border-left: 6px solid rgb(58, 58, 58); "
    "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "
    "background-color: lightblue;";

constexpr auto SETTING_KEY_N = "N";
constexpr auto SETTING_KEY_KEEP_ISOLATED = "KeepIsolated";

using PLUG::ApplySettingsResult;
using PLUG::Error;
using PLUG::ErrorCode;
using PLUG::InputData;
using PLUG::InputLoadedResult;
using PLUG::SelectionData;
using PLUG::SelectionResult;
using PLUG::SettingsWidgetResult;

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

SettingsWidgetResult NthFrame::getSettingsWidget() {
    return createSettingsWidget();
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

    emit syncSettingsWidget(m_n, m_keepIsolated);

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
    emit syncSettingsWidget(m_n, m_keepIsolated);
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

std::unique_ptr<QWidget> NthFrame::createSettingsWidget() {
    auto settingsWidget = std::make_unique<QWidget>(nullptr);
    auto* layout = new QVBoxLayout(settingsWidget.get());
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create N value selector
    auto* nSelectorWidget = new QWidget(settingsWidget.get());
    auto* nSelectorLayout = new QHBoxLayout(nSelectorWidget);
    nSelectorLayout->setSpacing(0);
    nSelectorLayout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(tr("Select every Nth image"), settingsWidget.get());
    nSelectorLayout->addWidget(label);

    auto* spinBox = new QSpinBox(settingsWidget.get());
    spinBox->setMinimum(1);
    spinBox->setMaximum(9999);
    spinBox->setValue(static_cast<int>(m_n));
    spinBox->setAlignment(Qt::AlignRight);
    nSelectorLayout->addWidget(spinBox);

    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &NthFrame::slot_nChanged);

    layout->addWidget(nSelectorWidget);

    // Create "keep isolated" checkbox
    auto* checkBox = new QCheckBox(tr("Keep isolated images"), settingsWidget.get());
    checkBox->setChecked(m_keepIsolated);
    connect(checkBox, &QCheckBox::toggled, this, &NthFrame::slot_checkboxToggled);
    layout->addWidget(checkBox);

    connect(this, &NthFrame::syncSettingsWidget, settingsWidget.get(),
            [spinBox, checkBox](uint n, bool keepIsolated) {
                QSignalBlocker spinBlocker(spinBox);
                QSignalBlocker checkBlocker(checkBox);
                spinBox->setValue(static_cast<int>(n));
                checkBox->setChecked(keepIsolated);
            },
            Qt::QueuedConnection);

    // Add description
    auto* descriptionLabel = new QLabel(
        tr("When selecting strictly every Nth frame, isolated images or "
           "small batches are unlikely to get selected for larger N."),
        settingsWidget.get());
    descriptionLabel->setStyleSheet(DESCRIPTION_STYLE);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);

    settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    settingsWidget->adjustSize();
    emit syncSettingsWidget(m_n, m_keepIsolated);
    return settingsWidget;
}

