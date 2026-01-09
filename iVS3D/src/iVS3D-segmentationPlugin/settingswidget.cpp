#include "settingswidget.h"

using namespace segmentationplugin;

SettingsWidget::SettingsWidget(const QString& error_msg) 
    : QWidget(nullptr), m_models()
    , m_stackedWidget(new QStackedWidget(this))
    , m_settingsPage(new QWidget(this))
    , m_errorPage(new QWidget(this))
{
    setupUI();
    displayError(error_msg);
}

SettingsWidget::SettingsWidget(const QVector<ModelInfo>& models) 
    : QWidget(nullptr), m_models(models)
    , m_stackedWidget(new QStackedWidget(this))
    , m_settingsPage(new QWidget(this))
    , m_errorPage(new QWidget(this))
{
    std::printf("SettingsWidget: Loaded %d models.\n", m_models.size());
    setupUI();
    if (m_models.isEmpty()) {
        displayError(tr("No models available."));
        return;
    }

    m_modelComboBox->clear();
    for (const auto& model : m_models) {
        m_modelComboBox->addItem(model.name);
    }

    // Load classes for the first model by default
    const auto& firstModel = m_models.front();
    //m_classLayout->setParent(nullptr); // Clear previous layout
    setModel(firstModel);

    displaySettings();
}

void SettingsWidget::setupUI() {
    m_stackedWidget->addWidget(m_settingsPage);
    m_stackedWidget->addWidget(m_errorPage);
    m_stackedWidget->setCurrentWidget(m_settingsPage);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_stackedWidget);

    // Settings page layout
    auto settingsLayout = new QFormLayout(m_settingsPage);
    m_modelComboBox = new QComboBox(m_settingsPage);
    settingsLayout->addRow(tr("Select Model:"), m_modelComboBox);
    connect(m_modelComboBox, &QComboBox::currentTextChanged, [this](const QString& text){
        emit modelChanged(text);
    });

    m_alphaSlider = new QSlider(Qt::Horizontal, m_settingsPage);
    m_alphaSlider->setRange(0, 100);
    settingsLayout->addRow(tr("Overlay Alpha:"), m_alphaSlider);
    connect(m_alphaSlider, &QSlider::valueChanged, [this](int value){
        emit overlayAlphaChanged(static_cast<float>(value) / 100.0f);
    });
    
    m_classView = std::make_unique<ModelClassView>(QVector<ModelClass>{}, m_settingsPage);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    settingsLayout->addRow(tr("Select Classes:"), m_classView.get());

    // Error page layout
    auto errorLayout = new QVBoxLayout(m_errorPage);
    m_errorLabel = new QLabel(m_errorPage);
    errorLayout->addWidget(m_errorLabel);
}

void SettingsWidget::setModel(const ModelInfo& model) {
    m_settingsPage->layout()->removeWidget( m_classView.get());
    disconnect(m_classView.get(), &ModelClassView::classSelectionChanged,
               this, &SettingsWidget::onClassSelectionChanged);

    m_classView = std::make_unique<ModelClassView>(model.classes, m_settingsPage);
    connect(m_classView.get(), &ModelClassView::classSelectionChanged,
            this, &SettingsWidget::onClassSelectionChanged);
    m_settingsPage->layout()->addWidget(m_classView.get());
}

void SettingsWidget::setOverlayAlpha(float alpha) {
    m_alphaSlider->setValue(static_cast<int>(alpha * 100));
}

void SettingsWidget::displayError(const QString& message) {
    m_errorLabel->setText(message);
    m_stackedWidget->setCurrentWidget(m_errorPage);
}

void SettingsWidget::displaySettings() {
    m_stackedWidget->setCurrentWidget(m_settingsPage);
}