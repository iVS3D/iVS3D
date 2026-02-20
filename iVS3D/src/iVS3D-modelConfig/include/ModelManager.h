#pragma once

#include <memory>
#include <optional>

#include <QObject>
#include <QHash>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

#include <tl/expected.hpp>

#include <ModelConfig.h>

class ModelManager : public QObject {
    Q_OBJECT

public:
    enum class ModelState {
        Ready = 0,
        MissingConfig = 1,
        MissingModel = 2,
        InvalidConfig = 3,
        Incompatible = 4
    };

    struct ModelEntry {
        QString name;
        QString jsonPath;
        QString onnxPath;
        ModelState state = ModelState::MissingConfig;
        QString error;
        std::shared_ptr<ModelConfig> config;
    };

    explicit ModelManager(QString modelDir = defaultModelDirectory());
    ~ModelManager();

    static QString defaultModelDirectory();

    /**
     * @brief Set a filename filter pattern (wildcard style)
     * Only model configs matching this pattern will be loaded.
     * Uses wildcard matching (e.g., "Detection_*", "*yolo*")
     * By default (empty string), all models are loaded.
     * 
     * @param pattern Wildcard pattern (e.g., "Detection_*")
     */
    void setNameFilter(const QString& pattern);

    /**
     * @brief Get the current name filter pattern
     * @return The wildcard pattern, or empty string if no filter is set
     */
    QString nameFilter() const noexcept;

    void refresh();

    const QVector<ModelEntry>& models() const noexcept;

    QStringList availableModelNames() const;

    ModelState modelState(const QString& name) const noexcept;

    QString modelError(const QString& name) const noexcept;

    std::optional<ModelEntry> activateModel(const QString& name);

    std::optional<ModelEntry> activeModel() const noexcept;

    const QString& activeModelName() const noexcept;

    bool setClassSelected(const QString& modelName,
                          ModelConfig::ClassId id, bool selected);

    void markModelIncompatible(const QString& name, const QString& reason);

    QString modelToString(const QString& name) const noexcept;
    QJsonObject modelToJson(const QString& name) const noexcept;
    std::optional<ModelEntry> modelFromJson(const QJsonObject& obj) const noexcept;

signals:
    /**
     * @brief Emitted when a model is activated and initial data is ready
     * @param modelName Name of the activated model
     * @param state Current state of the model
     * @param error Error message if state is not Ready, otherwise empty
     */
    void modelActivated(const QString& modelName, ModelManager::ModelState state, const QString& error);

    /**
     * @brief Emitted when class list is updated for the current model
     * @param classIds Vector of class IDs for the current model
     */
    void classListUpdated(const QVector<ModelConfig::ClassInfo>& classes);

    /**
     * @brief Emitted when models list is refreshed
     * @param models The updated list of models
     */
    void modelsListUpdated(const QVector<ModelEntry>& models);

public slots:
    /**
     * @brief Slot to handle model activation requests from UI thread
     * @param modelName Name of the model to activate
     */
    void onModelActivationRequested(const QString& modelName);

    /**
     * @brief Slot to handle class selection requests from UI thread
     * @param modelName Name of the model
     * @param classId ID of the class
     * @param selected Whether the class should be selected
     */
    void onClassSelectionRequested(const QString& modelName, uint classId, bool selected);

    /**
     * @brief Slot to handle refresh requests from UI thread
     */
    void onModelsRefreshRequested();

    /**
     * @brief Slot to handle apply mean/std setting changes from UI thread
     * @param modelName Name of the model
     * @param apply Whether to apply mean/std normalization
     */
    void onApplyMeanStdRequested(const QString& modelName, bool apply);

    /**
     * @brief Slot to handle normalize to [0,1] setting changes from UI thread
     * @param modelName Name of the model
     * @param normalize Whether to normalize input to [0,1]
     */
    void onNormalizeTo01Requested(const QString& modelName, bool normalize);

    /**
     * @brief Slot to handle input alignment changes from UI thread
     * @param modelName Name of the model
     * @param alignment Input alignment value
     */
    void onInputAlignmentRequested(const QString& modelName, uint alignment);

private:
    const ModelEntry* findEntry(const QString& name) const noexcept;

    QString modelDir_;
    QString nameFilter_;  // Wildcard pattern for filtering models
    QVector<ModelEntry> models_;
    QHash<QString, int> indexByName_;

    QString activeModelName_;
};

// Register custom types for use in signals/slots
Q_DECLARE_METATYPE(ModelManager::ModelState)
Q_DECLARE_METATYPE(ModelManager::ModelEntry)
Q_DECLARE_METATYPE(QVector<ModelManager::ModelEntry>)
