#pragma once

#include <memory>
#include <optional>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

#include <tl/expected.hpp>

#include <ModelConfig.h>

class ModelManager {
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


private:
    const ModelEntry* findEntry(const QString& name) const noexcept;

    QString modelDir_;
    QString nameFilter_;  // Wildcard pattern for filtering models
    QVector<ModelEntry> models_;
    QHash<QString, int> indexByName_;

    QString activeModelName_;
};
