#include "ModelSettingsWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QFrame>
#include <QIntValidator>
#include <QGroupBox>
#include <QEvent>
#include <QMouseEvent>


ModelSettingsWidget::ModelSettingsWidget(
    ModelManager& manager, QWidget* parent)
    : QWidget(parent), m_manager(manager)
{
    static bool firstInstance = true;
    if (firstInstance) {
        // Register metatypes for use in signals/slots across threads
        qRegisterMetaType<QVector<uint>>("QVector<uint>");
        qRegisterMetaType<ModelManager::ModelState>("ModelManager::ModelState");
        qRegisterMetaType<QVector<ModelConfig::ClassInfo>>("QVector<ModelConfig::ClassInfo>");
        qRegisterMetaType<QVector<ModelManager::ModelEntry>>("QVector<ModelManager::ModelEntry>");
        firstInstance = false;
    }
    setupUi();
    setupManagerConnections();
    
    // Request initial models list from manager
    emit modelsRefreshRequested();
}

void ModelSettingsWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ==================== Model Selection ====================
    auto* modelLayout = new QHBoxLayout();
    auto* modelLabel = new QLabel(tr("Model:"), this);
    
    m_modelCombo = new QComboBox(this);
    m_modelCombo->setToolTip(tr("Select which neural network to use"));
    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: #666; }");
    
    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(m_modelCombo, 1);
    modelLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(modelLayout);

    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ModelSettingsWidget::onModelIndexChanged);

    // ==================== Class Selection Container ====================
    m_classContainer = new QWidget(this);
    auto* classLayout = new QVBoxLayout(m_classContainer);
    classLayout->setContentsMargins(0, 0, 0, 0);
    classLayout->setSpacing(8);

    // Search and invert controls
    auto* classHeaderLayout = new QHBoxLayout();
    auto* classLabel = new QLabel(tr("Select Classes:"), this);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(150);
    
    m_invertButton = new QPushButton(tr("Invert"), this);
    m_invertButton->setMaximumWidth(80);
    
    classHeaderLayout->addWidget(classLabel);
    classHeaderLayout->addStretch();
    classHeaderLayout->addWidget(m_searchEdit);
    classHeaderLayout->addWidget(m_invertButton);
    classLayout->addLayout(classHeaderLayout);

    connect(m_invertButton, &QPushButton::clicked,
            this, &ModelSettingsWidget::onInvertSelectionClicked);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ModelSettingsWidget::onSearchTextChanged);

    // Class grid
    m_classGridWidget = new QWidget(this);
    auto* gridLayout = new QGridLayout(m_classGridWidget);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    
    m_classScrollArea = new QScrollArea(this);
    m_classScrollArea->setWidget(m_classGridWidget);
    m_classScrollArea->setWidgetResizable(true);
    m_classScrollArea->setMinimumHeight(150);
    m_classScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ccc; border-radius: 4px; }");
    classLayout->addWidget(m_classScrollArea, 1);

    mainLayout->addWidget(m_classContainer, 1);

    // ==================== Configuration Section (Collapsible) ====================
    m_configurationSection = new QWidget(this);
    auto* configLayout = new QVBoxLayout(m_configurationSection);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->setSpacing(0);

    // Toggle button for collapsible section
    auto* configHeaderLayout = new QHBoxLayout();
    m_configSectionToggleButton = new QPushButton(tr("▶  Model Configuration"), this);
    m_configSectionToggleButton->setFlat(true);
    m_configSectionToggleButton->setCursor(Qt::PointingHandCursor);
    m_configSectionToggleButton->setStyleSheet(
        "QPushButton { text-align: left; border: None; }");
    
    configHeaderLayout->addWidget(m_configSectionToggleButton);
    configHeaderLayout->addStretch();
    configLayout->addLayout(configHeaderLayout);
    
    connect(m_configSectionToggleButton, &QPushButton::clicked,
            this, [this]() {
        m_configurationSectionExpanded = !m_configurationSectionExpanded;
        QString arrow = m_configurationSectionExpanded ? tr("▼") : tr("▶");
        m_configSectionToggleButton->setText(arrow + tr("  Model Configuration"));
        // Toggle visibility of the content
        if (m_configContentWidget) {
            m_configContentWidget->setVisible(m_configurationSectionExpanded);
        }
    });

    // Configuration content (initially hidden) - using QGroupBox for consistent styling
    m_configContentWidget = new QGroupBox(this);
    m_configContentWidget->setTitle(""); // No title, controlled by toggle button
    auto* configContentLayout = new QVBoxLayout(m_configContentWidget);
    configContentLayout->setContentsMargins(12, 12, 12, 12);
    configContentLayout->setSpacing(10);

    // Apply Mean/Std
    auto* applyMeanStdLayout = new QHBoxLayout();
    m_applyMeanStdCheckBox = new QCheckBox(tr("Apply Mean/Std Normalization"), this);
    m_applyMeanStdCheckBox->setToolTip(tr("Apply mean and standard deviation normalization"));
    m_meanStdDisplayLabel = new QLabel(this);
    m_meanStdDisplayLabel->setStyleSheet("QLabel { color: #666; font-family: monospace; font-size: 10px; }");
    applyMeanStdLayout->addWidget(m_applyMeanStdCheckBox);
    applyMeanStdLayout->addStretch();
    applyMeanStdLayout->addWidget(m_meanStdDisplayLabel);
    configContentLayout->addLayout(applyMeanStdLayout);

    connect(m_applyMeanStdCheckBox, &QCheckBox::toggled,
            this, &ModelSettingsWidget::onApplyMeanStdToggled);

    // Normalize to [0,1]
    m_normalizeTo01CheckBox = new QCheckBox(tr("Normalize input [0,255] → [0,1]"), this);
    m_normalizeTo01CheckBox->setToolTip(tr("Divide pixel values by 255 before applying normalization"));
    configContentLayout->addWidget(m_normalizeTo01CheckBox);

    connect(m_normalizeTo01CheckBox, &QCheckBox::toggled,
            this, &ModelSettingsWidget::onNormalizeTo01Toggled);

    // Input Alignment
    auto* inputAlignmentLayout = new QHBoxLayout();
    auto* alignmentLabel = new QLabel(tr("Input Alignment:"), this);
    m_inputAlignmentEdit = new QLineEdit(this);
    m_inputAlignmentEdit->setMaximumWidth(100);
    m_inputAlignmentEdit->setValidator(new QIntValidator(1, 1024, this));
    inputAlignmentLayout->addWidget(alignmentLabel);
    inputAlignmentLayout->addWidget(m_inputAlignmentEdit);
    inputAlignmentLayout->addStretch();
    configContentLayout->addLayout(inputAlignmentLayout);

    connect(m_inputAlignmentEdit, &QLineEdit::textChanged,
            this, &ModelSettingsWidget::onInputAlignmentChanged);

    // Configuration change warning
    m_configChangeWarningLabel = new QLabel(this);
    m_configChangeWarningLabel->setWordWrap(true);
    m_configChangeWarningLabel->setStyleSheet(
        "QLabel { color: #856404;"
        "border: 1px solid #ffc107; border-radius: 4px; padding: 8px; }");
    m_configChangeWarningLabel->setText(
        tr("ⓘ Configuration changes made here are temporary and will not be persisted. "
           "To save these settings permanently, manually update the model configuration JSON file."));
    m_configChangeWarningLabel->hide();
    configContentLayout->addWidget(m_configChangeWarningLabel);

    configLayout->addWidget(m_configContentWidget);
    configLayout->addStretch();

    mainLayout->addWidget(m_configurationSection, 0);
    m_configurationSection->hide();
    m_configContentWidget->hide();


    // ==================== Error Display Container ====================
    m_errorContainer = new QWidget(this);
    auto* errorLayout = new QVBoxLayout(m_errorContainer);
    errorLayout->setContentsMargins(12, 12, 12, 12);
    errorLayout->setSpacing(8);

    auto* errorFrame = new QFrame(this);
    errorFrame->setFrameShape(QFrame::StyledPanel);
    errorFrame->setStyleSheet(
        "QFrame { background-color: #fff3cd; border: 1px solid #ffc107; "
        "border-radius: 4px; padding: 8px; }");
    auto* errorFrameLayout = new QVBoxLayout(errorFrame);
    
    m_errorMessageLabel = new QLabel(this);
    m_errorMessageLabel->setWordWrap(true);
    m_errorMessageLabel->setStyleSheet("QLabel { color: #856404; font-weight: bold; }");
    
    m_errorHintLabel = new QLabel(this);
    m_errorHintLabel->setWordWrap(true);
    m_errorHintLabel->setStyleSheet("QLabel { color: #856404; }");
    
    errorFrameLayout->addWidget(m_errorMessageLabel);
    errorFrameLayout->addWidget(m_errorHintLabel);
    
    errorLayout->addWidget(errorFrame);
    errorLayout->addStretch();
    
    mainLayout->addWidget(m_errorContainer, 1);

    // Initially hide both containers
    m_classContainer->hide();
    m_errorContainer->hide();
}

void ModelSettingsWidget::setupManagerConnections()
{
    // Connect widget signals to manager slots (crosses thread boundary)
    connect(this, &ModelSettingsWidget::modelActivationRequested,
            &m_manager, &ModelManager::onModelActivationRequested,
            Qt::QueuedConnection);
    
    connect(this, &ModelSettingsWidget::classSelectionRequested,
            &m_manager, &ModelManager::onClassSelectionRequested,
            Qt::QueuedConnection);
    
    connect(this, &ModelSettingsWidget::modelsRefreshRequested,
            &m_manager, &ModelManager::onModelsRefreshRequested,
            Qt::QueuedConnection);
    
    connect(this, &ModelSettingsWidget::applyMeanStdRequested,
            &m_manager, &ModelManager::onApplyMeanStdRequested,
            Qt::QueuedConnection);
    
    connect(this, &ModelSettingsWidget::normalizeTo01Requested,
            &m_manager, &ModelManager::onNormalizeTo01Requested,
            Qt::QueuedConnection);
    
    connect(this, &ModelSettingsWidget::inputAlignmentRequested,
            &m_manager, &ModelManager::onInputAlignmentRequested,
            Qt::QueuedConnection);
    
    // Connect manager signals to widget slots (crosses thread boundary)
    connect(&m_manager, QOverload<const QString&, ModelManager::ModelState, const QString&>::of(&ModelManager::modelActivated),
            this, &ModelSettingsWidget::onModelActivated,
            Qt::QueuedConnection);
    
    connect(&m_manager, QOverload<const QVector<ModelConfig::ClassInfo>&>::of(&ModelManager::classListUpdated),
            this, &ModelSettingsWidget::onClassListUpdated,
            Qt::QueuedConnection);
    
    connect(&m_manager, QOverload<const QVector<ModelManager::ModelEntry>&>::of(&ModelManager::modelsListUpdated),
            this, &ModelSettingsWidget::onModelsListUpdated,
            Qt::QueuedConnection);
}

void ModelSettingsWidget::refreshModelList()
{
    m_blockSignals = true;
    
    QString previousSelection = m_currentModelName;
    m_modelCombo->clear();
    
    const auto& models = m_cachedModels;
    
    if (models.isEmpty()) {
        m_modelCombo->addItem(tr("No models found"));
        m_modelCombo->setEnabled(false);
        m_statusLabel->setText(tr("No detection models available"));
        m_classContainer->hide();
        m_errorContainer->show();
        showModelError(ModelManager::ModelState::MissingConfig,
                      tr("No detection models found in the model directory."));
        m_blockSignals = false;
        return;
    }
    
    m_modelCombo->setEnabled(true);
    
    // Add all models to dropdown with status indicators
    int previousIndex = -1;
    for (int i = 0; i < models.size(); ++i) {
        const auto& entry = models[i];
        QString displayName = entry.name;
        
        // Add visual indicator for invalid models
        if (entry.state != ModelManager::ModelState::Ready) {
            displayName += QString(" [%1]").arg(getStateString(entry.state));
        }
        
        m_modelCombo->addItem(displayName, entry.name);
        
        if (entry.name == previousSelection) {
            previousIndex = i;
        }
    }
    
    // Restore previous selection or select first valid model
    if (previousIndex >= 0) {
        m_modelCombo->setCurrentIndex(previousIndex);
    } else {
        // Try to select first valid model
        int firstValidIndex = -1;
        for (int i = 0; i < models.size(); ++i) {
            if (models[i].state == ModelManager::ModelState::Ready) {
                firstValidIndex = i;
                break;
            }
        }
        m_modelCombo->setCurrentIndex(firstValidIndex >= 0 ? firstValidIndex : 0);
    }
    
    m_blockSignals = false;
    
    // Request activation of the selected model
    // This is needed because setCurrentIndex() while m_blockSignals was true won't trigger onModelIndexChanged()
    int currentIndex = m_modelCombo->currentIndex();
    if (currentIndex >= 0) {
        QString modelName = m_modelCombo->itemData(currentIndex).toString();
        if (!modelName.isEmpty()) {
            m_currentModelName = modelName;
            emit modelActivationRequested(modelName);
        }
    }
}

bool ModelSettingsWidget::setSelectedModel(const QString& modelName)
{
    for (int i = 0; i < m_modelCombo->count(); ++i) {
        if (m_modelCombo->itemData(i).toString() == modelName) {
            m_blockSignals = true;
            m_modelCombo->setCurrentIndex(i);
            m_blockSignals = false;
            updateModelDisplay();
            return true;
        }
    }
    return false;
}

QString ModelSettingsWidget::selectedModel() const
{
    //assert(m_currentModelName == m_manager.activeModelName());
    return m_currentModelName;
}

QVector<uint> ModelSettingsWidget::selectedClassIds() const
{
    QVector<uint> selected;
    for (int i = 0; i < m_classCheckBoxes.size(); ++i) {
        if (m_classCheckBoxes[i]->isChecked() && i < m_classIds.size()) {
            selected.append(m_classIds[i]);
        }
    }
    return selected;
}

void ModelSettingsWidget::setSelectedClassIds(const QVector<uint>& classIds)
{
    m_blockSignals = true;
    
    for (int i = 0; i < m_classCheckBoxes.size(); ++i) {
        if (i < m_classIds.size()) {
            bool shouldBeChecked = classIds.contains(m_classIds[i]);
            m_classCheckBoxes[i]->setChecked(shouldBeChecked);
        }
    }
    
    m_blockSignals = false;
}

void ModelSettingsWidget::onModelIndexChanged(int index)
{
    if (m_blockSignals || index < 0) {
        return;
    }
    
    QString modelName = m_modelCombo->itemData(index).toString();
    if (!modelName.isEmpty()) {
        m_currentModelName = modelName;
        emit modelActivationRequested(modelName);
    }
}

void ModelSettingsWidget::updateModelDisplay()
{
    // This is now called after receiving cached data from manager signals
    if (m_cachedModelState == ModelManager::ModelState::Ready) {
        // Valid model - show classes
        m_statusLabel->setText(tr("Ready"));
        m_statusLabel->setStyleSheet("QLabel { color: #28a745; font-weight: bold; }");
        m_errorContainer->hide();
        m_classContainer->show();
        // Note: updateClassList() will be called by onClassListUpdated() after signal chain completes
        
        // Update normalization checkbox from cached config
        if (m_cachedCurrentConfig) {
            
            // Update configuration section
            updateConfigurationSection();
        }
        
        if (!m_blockSignals) {
            emit modelChanged(m_currentModelName, true);
        }
    } else {
        // Invalid model - show error
        m_statusLabel->setText(getStateString(m_cachedModelState));
        m_statusLabel->setStyleSheet("QLabel { color: #dc3545; font-weight: bold; }");
        m_classContainer->hide();
        m_errorContainer->show();
        m_configurationSection->hide();
        showModelError(m_cachedModelState, m_cachedModelError);
        
        if (!m_blockSignals) {
            emit modelChanged(m_currentModelName, false);
        }
    }
}

void ModelSettingsWidget::updateClassList()
{
    // Clear existing checkboxes
    for (auto* checkbox : m_classCheckBoxes) {
        disconnect(checkbox, nullptr, this, nullptr);
        delete checkbox;
    }
    m_classCheckBoxes.clear();
    
    for (auto* container : m_classContainers) {
        delete container;
    }
    m_classContainers.clear();
    m_classIds.clear();

    // Clear grid layout completely
    auto* gridLayout = qobject_cast<QGridLayout*>(m_classGridWidget->layout());
    if (gridLayout) {
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    const auto& classes = m_cachedClasses;
    
    if (classes.empty()) {
        if (gridLayout) {
            auto* emptyLabel = new QLabel(tr("No classes defined in this model"), this);
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
            gridLayout->addWidget(emptyLabel, 0, 0);
        }
        return;
    }
    
    // Create checkboxes for each class
    for (const auto& classInfo : classes) {
        QCheckBox* checkbox = new QCheckBox(classInfo.name, this);
        checkbox->setChecked(classInfo.selected);
        checkbox->setMinimumHeight(24);
        checkbox->setCursor(Qt::PointingHandCursor);
        
        // Style with class color
        QString styleSheet = QString(
            "QCheckBox {"
            "    border: 2px solid rgb(%1,%2,%3);"
            "    border-radius: 4px;"
            "    padding: 4px 6px;"
            "    background-color: transparent;"
            "    spacing: 6px;"
            "}")
            .arg(classInfo.color.red())
            .arg(classInfo.color.green())
            .arg(classInfo.color.blue());
        
        checkbox->setStyleSheet(styleSheet);
        
        m_classCheckBoxes.append(checkbox);
        m_classIds.append(classInfo.id);
        
        connect(checkbox, &QCheckBox::toggled,
                this, &ModelSettingsWidget::onClassCheckBoxToggled);
    }
    
    // Update display with search filter
    onSearchTextChanged(m_searchEdit->text());
}

void ModelSettingsWidget::showModelError(
    ModelManager::ModelState state, const QString& errorMessage)
{
    m_errorMessageLabel->setText(QString("⚠ %1").arg(errorMessage));
    m_errorHintLabel->setText(getResolutionHint(state));
}

QString ModelSettingsWidget::getResolutionHint(
    ModelManager::ModelState state) const
{
    switch (state) {
    case ModelManager::ModelState::MissingConfig:
        return tr("💡 Add a .json configuration file with the same name as the model in the models directory.");
    
    case ModelManager::ModelState::MissingModel:
        return tr("💡 Ensure the .onnx model file exists. Check the 'modelPath' in the config or "
                 "place a .onnx file with the same name as the config.");
    
    case ModelManager::ModelState::InvalidConfig:
        return tr("💡 Check for duplicate class IDs or mismatched normalization vector sizes (mean/std).");
    
    case ModelManager::ModelState::Incompatible:
        return tr("💡 This model may require a different version or configuration. Check the error details.");
    
    default:
        return tr("💡 Make sure to add your models and .json configurations to the models directory.");
    }
}

QString ModelSettingsWidget::getStateString(
    ModelManager::ModelState state) const
{
    switch (state) {
    case ModelManager::ModelState::Ready:
        return tr("Ready");
    case ModelManager::ModelState::MissingConfig:
        return tr("Missing Config");
    case ModelManager::ModelState::MissingModel:
        return tr("Missing Model");
    case ModelManager::ModelState::InvalidConfig:
        return tr("Invalid Config");
    case ModelManager::ModelState::Incompatible:
        return tr("Incompatible");
    default:
        return tr("Unknown");
    }
}

void ModelSettingsWidget::onClassCheckBoxToggled()
{
    if (!m_blockSignals) {
        // Emit signals to update model's class selection state on worker thread
        for (int i = 0; i < m_classCheckBoxes.size() && i < m_classIds.size(); ++i) {
            emit classSelectionRequested(m_currentModelName, m_classIds[i],
                                         m_classCheckBoxes[i]->isChecked());
        }
        
        emit classSelectionChanged(selectedClassIds());
    }
}

void ModelSettingsWidget::onInvertSelectionClicked()
{
    m_blockSignals = true;
    
    QString searchText = m_searchEdit->text().toLower();
    
    // Only invert visible (filtered) classes
    for (int i = 0; i < m_classCheckBoxes.size(); ++i) {
        bool visible = m_classContainers[i]->isVisible();
        if (visible) {
            m_classCheckBoxes[i]->setChecked(!m_classCheckBoxes[i]->isChecked());
        }
    }
    
    m_blockSignals = false;
    onClassCheckBoxToggled();  // Update and emit signal
}

void ModelSettingsWidget::onSearchTextChanged(const QString& text)
{
    QString searchText = text.toLower();
    
    // Clear and rebuild grid layout
    auto* gridLayout = qobject_cast<QGridLayout*>(m_classGridWidget->layout());
    if (!gridLayout) {
        gridLayout = new QGridLayout(m_classGridWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        // Note: Layout is already set by constructor, no need to call setLayout() again
    } else {
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item;
        }
    }
    
    // Reparent checkboxes to prevent deletion
    for (auto* checkbox : m_classCheckBoxes) {
        checkbox->setParent(nullptr);
    }
    
    // Delete old containers
    for (auto* container : m_classContainers) {
        delete container;
    }
    m_classContainers.clear();
    
    // Build matching list
    QVector<int> matchingIndices;
    for (int i = 0; i < m_classCheckBoxes.size(); ++i) {
        QString className = m_classCheckBoxes[i]->text();
        bool matches = searchText.isEmpty() || className.toLower().contains(searchText);
        if (matches) {
            matchingIndices.append(i);
        }
    }
    
    // Calculate columns
    int numMatching = matchingIndices.size();
    int columns = 2;
    if (numMatching > 30) columns = 3;
    else if (numMatching > 5) columns = 2;
    else columns = 1;
    
    // Add to grid
    for (int displayIdx = 0; displayIdx < matchingIndices.size(); ++displayIdx) {
        int originalIdx = matchingIndices[displayIdx];
        QCheckBox* checkbox = m_classCheckBoxes[originalIdx];
        
        auto* container = new QWidget();
        auto* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(checkbox);
        container->setCursor(Qt::PointingHandCursor);
        container->installEventFilter(this);
        m_classContainers.append(container);
        
        int row = displayIdx / columns;
        int col = displayIdx % columns;
        gridLayout->addWidget(container, row, col);
    }
    
    // Add stretch
    if (!matchingIndices.isEmpty()) {
        gridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
                           (numMatching / columns) + 1, 0, 1, columns);
    }
}

bool ModelSettingsWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget) {
            int index = m_classContainers.indexOf(widget);
            if (index >= 0 && index < m_classCheckBoxes.size()) {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_classCheckBoxes[index]->toggle();
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ModelSettingsWidget::onApplyMeanStdToggled(bool checked)
{
    m_cachedApplyMeanStd = checked;
    emit applyMeanStdRequested(m_currentModelName, checked);
    emit modelConfigChanged();
    checkConfigurationChanges();
}

void ModelSettingsWidget::onNormalizeTo01Toggled(bool checked)
{
    m_cachedNormalizeTo01 = checked;
    emit normalizeTo01Requested(m_currentModelName, checked);
    emit modelConfigChanged();
    checkConfigurationChanges();
}

void ModelSettingsWidget::onInputAlignmentChanged(const QString& text)
{
    if (!text.isEmpty()) {
        m_cachedInputAlignment = text.toUInt();
        emit inputAlignmentRequested(m_currentModelName, m_cachedInputAlignment);
        emit modelConfigChanged();
        checkConfigurationChanges();
    }
}

void ModelSettingsWidget::checkConfigurationChanges()
{
    // Check if any configuration differs from original
    bool hasChanges = (m_cachedApplyMeanStd != m_originalApplyMeanStd) ||
                      (m_cachedNormalizeTo01 != m_originalNormalizeTo01) ||
                      (m_cachedInputAlignment != m_originalInputAlignment);
    
    if (hasChanges) {
        m_configChangeWarningLabel->show();
    } else {
        m_configChangeWarningLabel->hide();
    }
}

void ModelSettingsWidget::updateConfigurationSection()
{
    if (!m_cachedCurrentConfig) {
        m_configurationSection->hide();
        return;
    }

    m_blockSignals = true;

    // Cache original values
    m_originalApplyMeanStd = m_cachedCurrentConfig->getApplyMeanStd();
    m_originalNormalizeTo01 = m_cachedCurrentConfig->getNormalizeTo01();
    m_originalInputAlignment = m_cachedCurrentConfig->getInputAlignment();

    m_cachedApplyMeanStd = m_originalApplyMeanStd;
    m_cachedNormalizeTo01 = m_originalNormalizeTo01;
    m_cachedInputAlignment = m_originalInputAlignment;
    m_cachedMean = m_cachedCurrentConfig->getMean();
    m_cachedStd = m_cachedCurrentConfig->getStd();

    // Update UI with config values
    m_applyMeanStdCheckBox->setChecked(m_originalApplyMeanStd);
    m_normalizeTo01CheckBox->setChecked(m_originalNormalizeTo01);
    m_inputAlignmentEdit->setText(QString::number(m_originalInputAlignment));

    // Update mean/std display
    QString meanStdText;
    if (!m_cachedMean.empty() && !m_cachedStd.empty()) {
        QStringList meanValues;
        for (float val : m_cachedMean) {
            meanValues << QString::number(val, 'f', 3);
        }
        QStringList stdValues;
        for (float val : m_cachedStd) {
            stdValues << QString::number(val, 'f', 3);
        }
        meanStdText = QString("μ=[%1] σ=[%2]")
            .arg(meanValues.join(", "))
            .arg(stdValues.join(", "));
    }
    m_meanStdDisplayLabel->setText(meanStdText);

    // Hide warning since we just loaded the original values
    m_configChangeWarningLabel->hide();

    // Reset expansion state and hide content by default
    m_configurationSectionExpanded = false;
    m_configSectionToggleButton->setText(tr("▶ Model Configuration"));
    if (m_configContentWidget) {
        m_configContentWidget->hide();
    }

    m_configurationSection->show();
    m_blockSignals = false;
}

void ModelSettingsWidget::onModelActivated(const QString& modelName, ModelManager::ModelState modelState, const QString& error)
{
    // Cache the response from manager
    m_cachedModelState = static_cast<ModelManager::ModelState>(modelState);
    m_cachedModelError = error;
    m_currentModelName = modelName;
    
    // Find the corresponding model entry to cache its config
    for (const auto& entry : m_cachedModels) {
        if (entry.name == modelName) {
            m_cachedCurrentConfig = entry.config;
            break;
        }
    }
    
    // Now update the display with cached data
    updateModelDisplay();
}

void ModelSettingsWidget::onClassListUpdated(const QVector<ModelConfig::ClassInfo>& classes)
{
    // Cache the class list from manager
    m_cachedClasses = classes;
    
    // Update the display with the new class list
    updateClassList();
}

void ModelSettingsWidget::onModelsListUpdated(const QVector<ModelManager::ModelEntry>& models)
{
    // Cache the models list from manager
    m_cachedModels = models;
    
    // Refresh the combo box display using cached data
    refreshModelList();
}