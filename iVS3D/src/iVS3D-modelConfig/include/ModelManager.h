#pragma once

/**
 * @file ModelManager.h
 * @brief Discovery, validation, activation, and state tracking for model configs.
 * @author Dominik Wüst
 * @date March 2026
 */

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

namespace MCFG {

/**
 * @class ModelManager
 * @brief Manages model configurations in a directory and exposes UI-friendly state.
 *
 * @details
 * The manager scans a model directory for JSON/ONNX pairs, validates each model
 * using ModelConfig::loadFromFile(), tracks per-model readiness/error states,
 * and emits Qt signals for GUI synchronization.
 *
 * The corresponding JSON schema is described in @ref modelconfig_readme "ModelConfig.md".
 *
 * @ingroup ModelConfig
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
class ModelManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief High-level availability state of a model entry.
     */
    enum class ModelState {
        Ready = 0,
        MissingConfig = 1,
        MissingModel = 2,
        InvalidConfig = 3,
        Incompatible = 4
    };

    /**
     * @brief Cached record for one discovered model.
     */
    struct ModelEntry {
        /**< Base model name (without extension). */
        QString name;
        /**< Absolute path to JSON config candidate. */
        QString jsonPath;
        /**< Absolute path to ONNX model candidate/resolved model path. */
        QString onnxPath;
        /**< Current validation state of this entry. */
        ModelState state = ModelState::MissingConfig;
        /**< Human-readable error details for non-ready states. */
        QString error;
        /**< Parsed configuration (present for `Ready` models). */
        std::shared_ptr<ModelConfig> config;
    };

    /**
     * @brief Construct a manager for a specific model directory.
     * @param modelDir Directory to scan. Defaults to defaultModelDirectory().
     */
    explicit ModelManager(QString modelDir = defaultModelDirectory());

    /** @brief Destructor. */
    ~ModelManager();

    /**
     * @brief Get the platform-specific default model directory.
     * @return Absolute path used by default constructor.
     */
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

    /**
     * @brief Re-scan the model directory and rebuild model list/state.
     */
    void refresh();

    /** @brief Get all discovered model entries (ready and invalid). */
    const QVector<ModelEntry>& models() const noexcept;

    /**
     * @brief Get sorted names of models currently in `Ready` state.
     * @return List of usable model names.
     */
    QStringList availableModelNames() const;

    /**
     * @brief Get state of a model by name.
     * @param name Model base name.
     * @return Current model state, or MissingConfig for unknown names.
     */
    ModelState modelState(const QString& name) const noexcept;

    /**
     * @brief Get error text of a model by name.
     * @param name Model base name.
     * @return Error string or a message indicating unknown model.
     */
    QString modelError(const QString& name) const noexcept;

    /**
     * @brief Activate a model and emit activation/class-list signals.
     * @param name Model base name.
     * @return Activated entry on success, std::nullopt otherwise.
     */
    std::optional<ModelEntry> activateModel(const QString& name);

    /**
     * @brief Get currently active model entry.
     * @return Active entry or std::nullopt if no valid model is active.
     */
    std::optional<ModelEntry> activeModel() const noexcept;

    /** @brief Get name of active model, or empty if none active. */
    const QString& activeModelName() const noexcept;

    /**
     * @brief Update selected state of one class of one model.
     * @param modelName Model base name.
     * @param id Class ID.
     * @param selected Desired selected state.
     * @return true if model/class was found and updated.
     */
    bool setClassSelected(const QString& modelName,
                          ModelConfig::ClassId id, bool selected);

    /**
     * @brief Mark a model as incompatible with runtime/model consumer.
     * @param name Model base name.
     * @param reason Human-readable incompatibility reason.
     */
    void markModelIncompatible(const QString& name, const QString& reason);

    /**
     * @brief Serialize model selection summary as user-friendly text.
     * @param name Model base name.
     * @return Description string, empty if model is unknown.
     */
    QString modelToString(const QString& name) const noexcept;

    /**
     * @brief Serialize model class-selection state to JSON.
     * @param name Model base name.
     * @return JSON object containing model name and selected classes.
     */
    QJsonObject modelToJson(const QString& name) const noexcept;

    /**
     * @brief Restore class-selection state from JSON for a known model.
     * @param obj JSON object previously produced by modelToJson().
     * @return Updated model entry, or std::nullopt if model name is unknown.
     */
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

} // namespace MCFG

// Register custom types for use in signals/slots
Q_DECLARE_METATYPE(MCFG::ModelManager::ModelState)
Q_DECLARE_METATYPE(MCFG::ModelManager::ModelEntry)
Q_DECLARE_METATYPE(QVector<MCFG::ModelManager::ModelEntry>)
