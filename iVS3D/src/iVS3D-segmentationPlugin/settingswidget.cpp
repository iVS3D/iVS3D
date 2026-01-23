#include "settingswidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSpacerItem>

using namespace segmentationplugin;

SettingsWidget::SettingsWidget(QWidget* parent, const QVector<ModelInfo>& models) 
    : QWidget(parent)
{
    setWindowTitle(tr("Segmentation Settings"));
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ==================== Model Selection ====================
    auto* modelLayout = new QHBoxLayout();
    auto* modelLabel = new QLabel(tr("Segmentation Model:"));
    modelLabel->setStyleSheet("font-weight: bold;");
    m_modelComboBox = new QComboBox(this);
    m_modelComboBox->setToolTip(tr("Select which segmentation model to use for inference"));
    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(m_modelComboBox, 1);
    mainLayout->addLayout(modelLayout);
    
    connect(m_modelComboBox, &QComboBox::currentTextChanged, [this](const QString& text){
        emit modelChanged(text);
    });

    // ==================== Overlay Alpha ====================
    auto* alphaLayout = new QHBoxLayout();
    auto* alphaLabel = new QLabel(tr("Overlay Opacity:"));
    alphaLabel->setStyleSheet("font-weight: bold;");
    m_alphaSlider = new QSlider(Qt::Horizontal, this);
    m_alphaSlider->setRange(0, 100);
    m_alphaSlider->setValue(50);
    m_alphaSlider->setToolTip(tr("Adjust transparency of the segmentation overlay (0% = transparent, 100% = opaque)"));
    m_alphaValue = new QLabel("0.50");
    m_alphaValue->setMinimumWidth(40);
    m_alphaValue->setAlignment(Qt::AlignCenter);
    alphaLayout->addWidget(alphaLabel);
    alphaLayout->addWidget(m_alphaSlider, 1);
    alphaLayout->addWidget(m_alphaValue);
    mainLayout->addLayout(alphaLayout);
    
    connect(m_alphaSlider, &QSlider::valueChanged, [this](int value){
        float alpha = static_cast<float>(value) / 100.0f;
        m_alphaValue->setText(QString::number(alpha, 'f', 2));
        emit overlayAlphaChanged(alpha);
    });

    // ==================== Class Selection Header ====================
    auto* classLabel = new QLabel(tr("<b>Select Classes to Mask:</b>"));
    classLabel->setToolTip(tr("Choose which segmentation classes will be included in the generated mask"));
    mainLayout->addWidget(classLabel);

    // ==================== Class Grid ====================
    m_classView = std::make_unique<ModelClassView>(QVector<ModelClass>{}, this);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(m_classView.get());
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(200);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #ccc; border-radius: 4px; }");
    mainLayout->addWidget(scrollArea, 1);

    m_modelComboBox->clear();
    for (const auto& model : models) {
        m_modelComboBox->addItem(model.name);
    }

    // Load classes for the first model by default
    if (!models.empty()) {
        const auto& firstModel = models.front();
        setModel(firstModel);
    }

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SettingsWidget::setModel(const ModelInfo& model) {
    disconnect(m_classView.get(), &ModelClassView::classSelectionChanged,
               this, &SettingsWidget::onClassSelectionChanged);

    m_classView = std::make_unique<ModelClassView>(model.classes, this);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    
    // Update the scroll area widget
    auto* scrollArea = findChild<QScrollArea*>();
    if (scrollArea) {
        scrollArea->setWidget(m_classView.get());
    }
}

void SettingsWidget::setOverlayAlpha(float alpha) {
    m_alphaSlider->blockSignals(true);
    m_alphaSlider->setValue(static_cast<int>(alpha * 100));
    m_alphaValue->setText(QString::number(alpha, 'f', 2));
    m_alphaSlider->blockSignals(false);
}