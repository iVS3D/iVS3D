#include "segsettingswidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QVBoxLayout>

SegmentationSettingsWidget::SegmentationSettingsWidget(ModelManager& manager,
                                                       QWidget* parent)
    : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_modelSettingsWidget = new ModelSettingsWidget(manager, this);
    mainLayout->addWidget(m_modelSettingsWidget);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    auto* alphaLayout = new QHBoxLayout();
    auto* alphaLabel = new QLabel(tr("Overlay Opacity:"), this);
    m_alphaSlider = new QSlider(Qt::Horizontal, this);
    m_alphaSlider->setRange(0, 100);
    m_alphaSlider->setToolTip(
        tr("Adjust transparency of the segmentation overlay (0% = "
           "transparent, 100% = opaque)"));
    m_alphaValue = new QLabel(this);
    m_alphaValue->setMinimumWidth(40);
    m_alphaValue->setAlignment(Qt::AlignCenter);

    alphaLayout->addWidget(alphaLabel);
    alphaLayout->addWidget(m_alphaSlider, 1);
    alphaLayout->addWidget(m_alphaValue);
    mainLayout->addLayout(alphaLayout);
    mainLayout->addStretch();

    connect(m_alphaSlider, &QSlider::valueChanged, this, [this](int value) {
        const float alpha = static_cast<float>(value) / 100.0f;
        m_alphaValue->setText(QString::number(alpha, 'f', 2));
        emit overlayAlphaChanged(alpha);
    });
}

void SegmentationSettingsWidget::setOverlayAlpha(float alpha) {
    QSignalBlocker blocker(m_alphaSlider);
    m_alphaSlider->setValue(static_cast<int>(alpha * 100.0f));
    m_alphaValue->setText(QString::number(alpha, 'f', 2));
}

float SegmentationSettingsWidget::overlayAlpha() const {
    return static_cast<float>(m_alphaSlider->value()) / 100.0f;
}

void SegmentationSettingsWidget::applyPluginSettings(
    const QString& selectedModelName,
    float overlayAlpha) {
    QSignalBlocker sliderBlocker(m_alphaSlider);
    m_alphaSlider->setValue(static_cast<int>(overlayAlpha * 100.0f));
    m_alphaValue->setText(QString::number(overlayAlpha, 'f', 2));

    m_modelSettingsWidget->setSelectedModel(selectedModelName);
}
