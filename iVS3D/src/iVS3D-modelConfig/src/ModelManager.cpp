#include "ModelManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QJsonArray>

ModelManager::ModelManager(QString modelDir)
    : modelDir_(std::move(modelDir)), nameFilter_() {
    refresh();
}

ModelManager::~ModelManager() = default;

QString ModelManager::defaultModelDirectory() {
    return QCoreApplication::applicationDirPath() +
           "/plugins/resources/neural_network_models";
}

void ModelManager::setNameFilter(const QString& pattern) {
    nameFilter_ = pattern;
    refresh();
}

QString ModelManager::nameFilter() const noexcept {
    return nameFilter_;
}

const QVector<ModelManager::ModelEntry>&
ModelManager::models() const noexcept {
    return models_;
}

QStringList ModelManager::availableModelNames() const {
    QStringList names;
    for (const auto& entry : models_) {
        if (entry.state == ModelState::Ready) {
            names.append(entry.name);
        }
    }
    names.sort();
    return names;
}

const ModelManager::ModelEntry*
ModelManager::findEntry(const QString& name) const noexcept {
    auto it = indexByName_.find(name);
    if (it == indexByName_.end()) {
        return nullptr;
    }
    return &models_[it.value()];
}

ModelManager::ModelState
ModelManager::modelState(const QString& name) const noexcept {
    const auto* entry = findEntry(name);
    if (!entry) {
        return ModelState::MissingConfig;
    }
    return entry->state;
}

QString ModelManager::modelError(const QString& name) const noexcept {
    const auto* entry = findEntry(name);
    if (!entry) {
        return QString("Unknown model: %1").arg(name);
    }
    return entry->error;
}

void ModelManager::refresh() {
    models_.clear();
    indexByName_.clear();

    QDir dir(modelDir_);
    if (!dir.exists()) {
        return;
    }

    QStringList jsons = dir.entryList(QStringList() << "*.json", QDir::Files);
    QStringList onnxs = dir.entryList(QStringList() << "*.onnx", QDir::Files);

    QSet<QString> baseNames;
    for (const auto& json : jsons) {
        baseNames.insert(QFileInfo(json).completeBaseName());
    }
    for (const auto& onnx : onnxs) {
        baseNames.insert(QFileInfo(onnx).completeBaseName());
    }

    QStringList sortedNames = baseNames.values();
    sortedNames.sort();

    for (const auto& base : sortedNames) {
        // Apply name filter if set
        if (!nameFilter_.isEmpty()) {
            QRegExp regExp(nameFilter_, Qt::CaseSensitive, QRegExp::Wildcard);
            if (!regExp.exactMatch(base)) {
                continue;
            }
        }
        ModelEntry entry;
        entry.name = base;
        entry.jsonPath = dir.filePath(base + ".json");
        entry.onnxPath = dir.filePath(base + ".onnx");

        if (!QFileInfo::exists(entry.jsonPath)) {
            entry.state = ModelState::MissingConfig;
            entry.error = QString("Config file not found: %1")
                              .arg(entry.jsonPath);
        } else {
            auto cfgExp = ModelConfig::loadFromFile(entry.jsonPath);
            if (!cfgExp) {
                switch (cfgExp.error().code)
                {
                case ModelConfig::ErrorCode::ConfigFileNotFound:
                    entry.state = ModelState::MissingConfig;
                    break;
                case ModelConfig::ErrorCode::ModelFileNotFound:
                    entry.state = ModelState::MissingModel;
                    break;
                case ModelConfig::ErrorCode::IoError:
                case ModelConfig::ErrorCode::DuplicateClassIds:
                case ModelConfig::ErrorCode::ConfigParseError:
                default:
                    entry.state = ModelState::InvalidConfig;
                    break;
                }
                entry.error = cfgExp.error().message;
            } else {
                // All validation is now done by ModelConfig::loadFromFile
                entry.state = ModelState::Ready;
                entry.onnxPath = QString::fromStdString(cfgExp->getModelPath());
                entry.config = std::make_shared<ModelConfig>(std::move(*cfgExp));
            }
        }

        indexByName_.insert(entry.name, models_.size());
        models_.push_back(std::move(entry));
    }
}

std::optional<ModelManager::ModelEntry>
ModelManager::activateModel(const QString& name) {
    if (name != activeModelName_) {
        activeModelName_.clear();
    }
    const ModelEntry* entry = findEntry(name);
    if (!entry || entry->state != ModelState::Ready) {
        return std::nullopt;
    }
    activeModelName_ = name;
    return *entry;
}

std::optional<ModelManager::ModelEntry>
ModelManager::activeModel() const noexcept {
    const ModelEntry* entry = findEntry(activeModelName_);
    if (!entry) {
        return std::nullopt;
    }
    return *entry;
}

const QString& ModelManager::activeModelName() const noexcept {
    return activeModelName_;
}

bool ModelManager::setClassSelected(const QString& modelName,
                                                   ModelConfig::ClassId id,
                                                   bool selected) {
    const ModelEntry* entry = findEntry(modelName);
    if (!entry || !entry->config) {
        return false;
    }

    return entry->config->setClassSelected(id, selected);
}

void ModelManager::markModelIncompatible(const QString& name,
                                                        const QString& reason) {
    auto it = indexByName_.find(name);
    if (it == indexByName_.end()) {
        return;
    }

    auto& entry = models_[it.value()];
    entry.state = ModelState::Incompatible;
    entry.error = reason;
}

QString ModelManager::modelToString(const QString& name) const noexcept {
    auto it = indexByName_.find(name);
    if (it == indexByName_.end()) {
        return QString();
    }
    const auto& entry = models_[it.value()];
    QStringList desc;
    desc << QString("Model: %1").arg(entry.name);
    QStringList selectedClassNames;
    for (const auto& cls : entry.config->getClasses()) {
        if (cls.selected) {
            selectedClassNames.append(cls.name);
        }
    }
    desc << QString("Selected Classes: %1")
                .arg(selectedClassNames.join(", "));
    return desc.join(" | ");
}

QJsonObject ModelManager::modelToJson(const QString& name) const noexcept {
    auto it = indexByName_.find(name);
    if (it == indexByName_.end()) {
        return QJsonObject();
    }
    const auto& entry = models_[it.value()];
    QJsonObject obj;
    obj["name"] = entry.name;
    QJsonArray classesArray;
    for (const auto& cls : entry.config->getClasses()) {
        QJsonObject clsObj;
        clsObj["id"] = static_cast<int>(cls.id);
        clsObj["selected"] = cls.selected;
        classesArray.append(clsObj);
    }
    obj["classes"] = classesArray;
    return obj;
}

std::optional<ModelManager::ModelEntry> ModelManager::modelFromJson(
    const QJsonObject& obj) const noexcept {
    QString name = obj["name"].toString();
    auto it = indexByName_.find(name);
    if (it != indexByName_.end()) {
        auto entry = models_[it.value()];
        if (obj.contains("classes") && obj["classes"].isArray()) {
            QJsonArray classesArray = obj["classes"].toArray();
            for (const auto& clsValue : classesArray) {
                if (clsValue.isObject()) {
                    QJsonObject clsObj = clsValue.toObject();
                    ModelConfig::ClassId id = static_cast<ModelConfig::ClassId>(clsObj["id"].toInt());
                    bool selected = clsObj["selected"].toBool();
                    entry.config->setClassSelected(id, selected);
                }
            }
        }
        return entry;
    }
    return std::optional<ModelEntry>();
}
void ModelManager::onModelActivationRequested(const QString& modelName) {
    auto result = activateModel(modelName);
    if (result.has_value()) {
        const auto& entry = result.value();
        emit modelActivated(modelName, entry.state, entry.error);
        
        // Also emit class list if model is valid
        if (entry.state == ModelState::Ready && entry.config) {
            // Convert std::vector to QVector for Qt signal/slot communication
            const auto& stdClasses = entry.config->getClasses();
            QVector<ModelConfig::ClassInfo> qClasses(stdClasses.begin(), stdClasses.end());
            emit classListUpdated(qClasses);
        }
    } else {
        emit modelActivated(modelName, ModelState::MissingConfig, "Model not found");
    }
}

void ModelManager::onClassSelectionRequested(const QString& modelName, uint classId, bool selected) {
    setClassSelected(modelName, classId, selected);
}

void ModelManager::onModelsRefreshRequested() {
    refresh();
    emit modelsListUpdated(models_);
}

void ModelManager::onApplyMeanStdRequested(const QString& modelName, bool apply) {
    auto it = indexByName_.find(modelName);
    if (it != indexByName_.end()) {
        const auto& entry = models_[it.value()];
        if (entry.config) {
            entry.config->setApplyMeanStd(apply);
        }
    }
}

void ModelManager::onNormalizeTo01Requested(const QString& modelName, bool normalize) {
    auto it = indexByName_.find(modelName);
    if (it != indexByName_.end()) {
        const auto& entry = models_[it.value()];
        if (entry.config) {
            entry.config->setNormalizeTo01(normalize);
        }
    }
}

void ModelManager::onInputAlignmentRequested(const QString& modelName, uint alignment) {
    auto it = indexByName_.find(modelName);
    if (it != indexByName_.end()) {
        const auto& entry = models_[it.value()];
        if (entry.config) {
            entry.config->setInputAlignment(alignment);
        }
    }
}