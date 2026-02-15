#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QString>
#include <QScrollArea>
#include <QCheckBox>
#include <memory>
#include <QMetaType>

// Define this to show detailed model configuration info
#define SHOW_MODEL_CONFIG_INFO

#include "ModelManager.h"

/**
 * @class ModelSettingsWidget
 * @brief Reusable UI widget for selecting detection models and configuring class selection.
 *
 * This widget integrates with ModelManager to:
 * - Display all available models (valid and invalid) in a dropdown
 * - Show classes as checkboxes for valid models
 * - Display error messages for invalid models with resolution hints
 * - Emit signals when model or class selection changes
 *
 * The widget automatically handles model state (Ready, MissingModel, InvalidConfig, etc.)
 * and provides appropriate feedback to the user.
 */
class ModelSettingsWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct the settings widget
     * @param manager Reference to the model manager (must outlive this widget)
     * @param parent Parent widget
     */
    explicit ModelSettingsWidget(ModelManager& manager,
                                          QWidget* parent = nullptr);
    ~ModelSettingsWidget() = default;

    /**
     * @brief Refresh the model list from the manager
     * Call this after the manager's refresh() method or when models change
     */
    void refreshModelList();

    /**
     * @brief Set the currently selected model
     * @param modelName Name of the model to select
     * @return true if model was found and selected, false otherwise
     */
    bool setSelectedModel(const QString& modelName);

    /**
     * @brief Get the currently selected model name
     * @return Name of the selected model, or empty string if none selected
     */
    QString selectedModel() const;

    /**
     * @brief Get the class selection state for the current model
     * @return Vector of class IDs that are currently selected
     */
    QVector<uint> selectedClassIds() const;

    /**
     * @brief Set which classes are selected (by class ID)
     * @param classIds Vector of class IDs to select
     */
    void setSelectedClassIds(const QVector<uint>& classIds);

signals:
    /**
     * @brief Emitted when user selects a different model
     * @param modelName Name of the newly selected model
     * @param isValid true if the model is valid and ready, false if it has errors
     */
    void modelChanged(const QString& modelName, bool isValid);

    /**
     * @brief Emitted when user changes class selection
     * @param selectedClassIds Vector of class IDs that are now selected
     */
    void classSelectionChanged(const QVector<uint>& selectedClassIds);

    /**
     * @brief Emitted when user changes input normalization setting
     * @param normalizeInput true if [0,255] to [0,1] normalization is enabled
     */
    void normalizationChanged(bool normalizeInput);

    // Internal signals for communicating with ModelManager on worker thread
    void modelActivationRequested(const QString& modelName);
    void classSelectionRequested(const QString& modelName, uint classId, bool selected);
    void modelsRefreshRequested();
    void normalizeInputRequested(const QString& modelName, bool normalizeInput);

private slots:
    // Slots to handle responses from ModelManager
    void onModelActivated(const QString& modelName, ModelManager::ModelState modelState, const QString& error);
    void onClassListUpdated(const QVector<ModelConfig::ClassInfo>& classes);
    void onModelsListUpdated(const QVector<ModelManager::ModelEntry>& models);
    void onNormalizationSettingUpdated(const QString& modelName, bool normalizeInput);

    // Slots to handle UI interactions
    void onModelIndexChanged(int index);
    void onClassCheckBoxToggled();
    void onInvertSelectionClicked();
    void onSearchTextChanged(const QString& text);
    void onNormalizeInputToggled(bool checked);

private:
    /**
     * @brief Update the display based on currently selected model
     */
    void updateModelDisplay();

    /**
     * @brief Rebuild the class checkbox list
     */
    void updateClassList();

    /**
     * @brief Show error message for invalid model
     */
    void showModelError(ModelManager::ModelState state,
                       const QString& errorMessage);

    /**
     * @brief Create all UI elements
     */
    void setupUi();

    /**
     * @brief Setup connections to ModelManager signals/slots
     */
    void setupManagerConnections();

    /**
     * @brief Get human-readable hint for resolving model errors
     */
    QString getResolutionHint(ModelManager::ModelState state) const;

    /**
     * @brief Get human-readable string for model state
     */
    QString getStateString(ModelManager::ModelState state) const;

    // UI Elements
    QComboBox* m_modelCombo = nullptr;
    QLabel* m_statusLabel = nullptr;
    QCheckBox* m_normalizeInputCheckBox = nullptr;
#ifdef SHOW_MODEL_CONFIG_INFO
    QLabel* m_configInfoLabel = nullptr;
#endif
    QWidget* m_classContainer = nullptr;
    QWidget* m_errorContainer = nullptr;
    QLabel* m_errorMessageLabel = nullptr;
    QLabel* m_errorHintLabel = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_invertButton = nullptr;
    QWidget* m_classGridWidget = nullptr;
    QScrollArea* m_classScrollArea = nullptr;

    // Data
    ModelManager& m_manager;
    QString m_currentModelName;
    QVector<QCheckBox*> m_classCheckBoxes;
    QVector<QWidget*> m_classContainers;
    QVector<uint> m_classIds;  // Maps checkbox index to class ID
    
    // Cache for model state (since we can't directly query manager from GUI thread)
    QVector<ModelManager::ModelEntry> m_cachedModels;
    QVector<ModelConfig::ClassInfo> m_cachedClasses;
    ModelManager::ModelState m_cachedModelState = ModelManager::ModelState::MissingConfig;
    QString m_cachedModelError;
    bool m_cachedNormalizeInput = false;
    std::shared_ptr<ModelConfig> m_cachedCurrentConfig;
    
    bool m_blockSignals = false;
};

Q_DECLARE_METATYPE(QVector<uint>)
