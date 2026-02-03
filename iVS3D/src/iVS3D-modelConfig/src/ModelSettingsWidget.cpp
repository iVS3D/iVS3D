#include "ModelSettingsWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QFrame>


ModelSettingsWidget::ModelSettingsWidget(
    ModelManager& manager, QWidget* parent)
    : QWidget(parent), m_manager(manager)
{
    setupUi();
    refreshModelList();
}

void ModelSettingsWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ==================== Model Selection ====================
    auto* modelLayout = new QHBoxLayout();
    auto* modelLabel = new QLabel(tr("Model:"));
    
    m_modelCombo = new QComboBox();
    m_modelCombo->setToolTip(tr("Select which neural network to use"));
    
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("QLabel { color: #666; }");
    
    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(m_modelCombo, 1);
    modelLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(modelLayout);

    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ModelSettingsWidget::onModelIndexChanged);


#ifdef SHOW_MODEL_CONFIG_INFO
    // ==================== Model Configuration Info ====================
    m_configInfoLabel = new QLabel();
    m_configInfoLabel->setStyleSheet("QLabel { color: #666; font-family: monospace; }");
    m_configInfoLabel->setWordWrap(true);
    m_configInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(m_configInfoLabel);
#endif

    // ==================== Normalization Option ====================
    m_normalizeInputCheckBox = new QCheckBox(tr("Normalize input [0,255] → [0,1]"));
    m_normalizeInputCheckBox->setToolTip(tr("Divide pixel values by 255 before applying mean/std normalization"));
    mainLayout->addWidget(m_normalizeInputCheckBox);

    connect(m_normalizeInputCheckBox, &QCheckBox::toggled,
            this, &ModelSettingsWidget::onNormalizeInputToggled);

    // ==================== Class Selection Container ====================
    m_classContainer = new QWidget();
    auto* classLayout = new QVBoxLayout(m_classContainer);
    classLayout->setContentsMargins(0, 0, 0, 0);
    classLayout->setSpacing(8);

    // Search and invert controls
    auto* classHeaderLayout = new QHBoxLayout();
    auto* classLabel = new QLabel(tr("Select Classes:"));
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(150);
    
    m_invertButton = new QPushButton(tr("Invert"));
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
    m_classGridWidget = new QWidget();
    auto* gridLayout = new QGridLayout(m_classGridWidget);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    
    m_classScrollArea = new QScrollArea();
    m_classScrollArea->setWidget(m_classGridWidget);
    m_classScrollArea->setWidgetResizable(true);
    m_classScrollArea->setMinimumHeight(150);
    m_classScrollArea->setStyleSheet("QScrollArea { border: 1px solid #ccc; border-radius: 4px; }");
    classLayout->addWidget(m_classScrollArea, 1);

    mainLayout->addWidget(m_classContainer, 1);

    // ==================== Error Display Container ====================
    m_errorContainer = new QWidget();
    auto* errorLayout = new QVBoxLayout(m_errorContainer);
    errorLayout->setContentsMargins(12, 12, 12, 12);
    errorLayout->setSpacing(8);

    auto* errorFrame = new QFrame();
    errorFrame->setFrameShape(QFrame::StyledPanel);
    errorFrame->setStyleSheet(
        "QFrame { background-color: #fff3cd; border: 1px solid #ffc107; "
        "border-radius: 4px; padding: 8px; }");
    auto* errorFrameLayout = new QVBoxLayout(errorFrame);
    
    m_errorMessageLabel = new QLabel();
    m_errorMessageLabel->setWordWrap(true);
    m_errorMessageLabel->setStyleSheet("QLabel { color: #856404; font-weight: bold; }");
    
    m_errorHintLabel = new QLabel();
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

void ModelSettingsWidget::refreshModelList()
{
    m_blockSignals = true;
    
    QString previousSelection = m_currentModelName;
    m_modelCombo->clear();
    
    const auto& models = m_manager.models();
    
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
    updateModelDisplay();
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
    
    updateModelDisplay();
}

void ModelSettingsWidget::updateModelDisplay()
{
    int index = m_modelCombo->currentIndex();
    if (index < 0) {
        m_classContainer->hide();
        m_errorContainer->hide();
        return;
    }
    
    QString modelName = m_modelCombo->itemData(index).toString();
    m_currentModelName = modelName;

    m_manager.activateModel(modelName);
    
    auto state = m_manager.modelState(modelName);
    
    if (state == ModelManager::ModelState::Ready) {
        // Valid model - show classes
        m_statusLabel->setText(tr("Ready"));
        m_statusLabel->setStyleSheet("QLabel { color: #28a745; font-weight: bold; }");
        m_errorContainer->hide();
        m_classContainer->show();
        updateClassList();
        
        // Update normalization checkbox from config
        auto modelEntry = m_manager.activeModel();
        if (modelEntry && modelEntry->config) {
            m_normalizeInputCheckBox->blockSignals(true);
            m_normalizeInputCheckBox->setChecked(modelEntry->config->getNormalizeInput());
            m_normalizeInputCheckBox->setEnabled(true);
            m_normalizeInputCheckBox->blockSignals(false);
            
#ifdef SHOW_MODEL_CONFIG_INFO
            // Build config info string
            const auto& config = *modelEntry->config;
            QString info;
            
            // Mean values
            const auto& mean = config.getMean();
            info += "Mean: [";
            for (size_t i = 0; i < mean.size(); ++i) {
                if (i > 0) info += ", ";
                info += QString::number(mean[i], 'f', 3);
            }
            info += "]";
            
            // Std values
            const auto& std = config.getStd();
            info += " | Std: [";
            for (size_t i = 0; i < std.size(); ++i) {
                if (i > 0) info += ", ";
                info += QString::number(std[i], 'f', 3);
            }
            info += "]";
            
            // Input alignment
            uint alignment = config.getInputAlignment();
            if (alignment > 1) {
                info += " | Align: " + QString::number(alignment);
            }
            
            m_configInfoLabel->setText(info);
            m_configInfoLabel->show();
#endif
        }
        
        if (!m_blockSignals) {
            emit modelChanged(modelName, true);
        }
    } else {
        // Invalid model - show error
        QString error = m_manager.modelError(modelName);
        m_statusLabel->setText(getStateString(state));
        m_statusLabel->setStyleSheet("QLabel { color: #dc3545; font-weight: bold; }");
        m_classContainer->hide();
        m_errorContainer->show();
        showModelError(state, error);
        
        // Disable normalization checkbox for invalid models
        m_normalizeInputCheckBox->setEnabled(false);
        
#ifdef SHOW_MODEL_CONFIG_INFO
        m_configInfoLabel->hide();
#endif
        
        if (!m_blockSignals) {
            emit modelChanged(modelName, false);
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
    
    // Get active model to access classes
    auto result = m_manager.activateModel(m_currentModelName);
    if (!result.has_value()) {
        return;
    }
    
    auto modelEntry = result.value();
    if (modelEntry.state != ModelManager::ModelState::Ready ||
        !modelEntry.config) {
        return;
    }

    const auto& classes = modelEntry.config->getClasses();
    
    if (classes.empty()) {
        auto* gridLayout = qobject_cast<QGridLayout*>(m_classGridWidget->layout());
        if (gridLayout) {
            auto* emptyLabel = new QLabel(tr("No classes defined in this model"));
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("QLabel { color: #666; font-style: italic; }");
            gridLayout->addWidget(emptyLabel, 0, 0);
        }
        return;
    }
    
    // Create checkboxes for each class
    for (const auto& classInfo : classes) {
        QCheckBox* checkbox = new QCheckBox(classInfo.name);
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
        // Update model's class selection state
        for (int i = 0; i < m_classCheckBoxes.size() && i < m_classIds.size(); ++i) {
            m_manager.setClassSelected(m_currentModelName, m_classIds[i],
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
        m_classGridWidget->setLayout(gridLayout);
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

void ModelSettingsWidget::onNormalizeInputToggled(bool checked)
{
    // Update the model config
    auto modelEntry = m_manager.activeModel();
    if (modelEntry && modelEntry->config) {
        modelEntry->config->setNormalizeInput(checked);
        emit normalizationChanged(checked);
    }
}
