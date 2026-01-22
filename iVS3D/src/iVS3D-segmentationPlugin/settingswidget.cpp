#include "settingswidget.h"

using namespace segmentationplugin;

SettingsWidget::SettingsWidget(QWidget* parent, const QVector<ModelInfo>& models) 
    : QWidget(parent)
{
    auto settingsLayout = new QFormLayout(this);
    this->setLayout(settingsLayout);
    m_modelComboBox = new QComboBox(this);
    settingsLayout->addRow(tr("Select Model:"), m_modelComboBox);
    connect(m_modelComboBox, &QComboBox::currentTextChanged, [this](const QString& text){
        emit modelChanged(text);
    });

    m_alphaSlider = new QSlider(Qt::Horizontal, this);
    m_alphaSlider->setRange(0, 100);
    settingsLayout->addRow(tr("Overlay Alpha:"), m_alphaSlider);
    connect(m_alphaSlider, &QSlider::valueChanged, [this](int value){
        emit overlayAlphaChanged(static_cast<float>(value) / 100.0f);
    });
    
    m_classView = std::make_unique<ModelClassView>(QVector<ModelClass>{}, this);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    settingsLayout->addRow(tr("Select Classes:"), m_classView.get());

    m_modelComboBox->clear();
    for (const auto& model : models) {
        m_modelComboBox->addItem(model.name);
    }

    // Load classes for the first model by default
    const auto& firstModel = models.front();
    setModel(firstModel);
}

void SettingsWidget::setModel(const ModelInfo& model) {
    this->layout()->removeWidget( m_classView.get());
    disconnect(m_classView.get(), &ModelClassView::classSelectionChanged,
               this, &SettingsWidget::onClassSelectionChanged);

    m_classView = std::make_unique<ModelClassView>(model.classes, this);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    this->layout()->addWidget(m_classView.get());
}

void SettingsWidget::setOverlayAlpha(float alpha) {
    m_alphaSlider->setValue(static_cast<int>(alpha * 100));
}