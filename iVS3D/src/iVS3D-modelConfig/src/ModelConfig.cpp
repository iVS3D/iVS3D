#include "ModelConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QSet>

tl::expected<ModelConfig, ModelConfig::Error> ModelConfig::loadFromFile(
    const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tl::unexpected(
            Error{ErrorCode::IoError,
                  QString("Failed to open config file: %1").arg(jsonPath)});
    }

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        return tl::unexpected(Error{ErrorCode::ConfigParseError,
                                    QString("JSON parse error in file %1: %2")
                                        .arg(jsonPath)
                                        .arg(err.errorString())});
    }

    QJsonObject obj = doc.object();
    ModelConfig cfg;

    // Load normalization parameters
    for (const auto& val : obj["mean"].toArray()) {
        cfg.mean_.push_back(static_cast<float>(val.toDouble()));
    }
    for (const auto& val : obj["std"].toArray()) {
        cfg.std_.push_back(static_cast<float>(val.toDouble()));
    }

    // Load model path from config
    QString modelPathStr = obj["modelPath"].toString().trimmed();
    
    QFileInfo configFileInfo(jsonPath);
    
    // If no model path is given, default to config path with .onnx extension
    if (modelPathStr.isEmpty()) {
        QString basePath = configFileInfo.absolutePath() + "/" + configFileInfo.baseName();
        modelPathStr = basePath + ".onnx";
    } else if (!QFileInfo(modelPathStr).isAbsolute()) {
        // Handle relative paths
        QString configDir = configFileInfo.absolutePath();
        modelPathStr = QDir(configDir).filePath(modelPathStr);
    }
    
    // Verify that the model file exists
    if (!QFile::exists(modelPathStr)) {
        return tl::unexpected(
            Error{ErrorCode::ModelFileNotFound,
                  QString("Model file not found: %1").arg(modelPathStr)});
    }
    
    cfg.modelPath_ = modelPathStr.toStdString();

    // Load resolution alignment
    if (obj.contains("inputAlignment")) {
        cfg.inputAlignment_ = static_cast<uint>(obj["inputAlignment"].toInt(1));
    }

    // Load class information and validate for duplicate IDs
    QSet<ClassId> seenIds;
    ModelConfig::ClassId autoId = 0;
    
    for (const auto& cls_val : obj["classes"].toArray()) {
        QJsonObject cls_obj = cls_val.toObject();
        ModelConfig::ClassInfo cls;

        // Determine class ID
        if (cls_obj.contains("id")) {
            cls.id = static_cast<ModelConfig::ClassId>(cls_obj["id"].toInt());
        } else {
            cls.id = autoId++;
        }

        // Check for duplicate IDs
        if (seenIds.contains(cls.id)) {
            return tl::unexpected(
                Error{ErrorCode::DuplicateClassIds,
                      QObject::tr("Duplicate class ID found: %1").arg(cls.id)});
        }
        seenIds.insert(cls.id);

        // Load class name or use ID-based default name
        if (cls_obj.contains("name")) {
            cls.name = cls_obj["name"].toString();
        } else {
            cls.name = QObject::tr("Unknown <%1>").arg(cls.id);
        }

        // Load color for this class
        auto c = cls_obj["color"].toArray();
        if (c.size() == 3) {
            cls.color = QColor(c[0].toInt(), c[1].toInt(), c[2].toInt());
        } else {
            // Deterministic pseudo-random color from HSV wheel (seeded by class id)
            const quint32 seed = static_cast<quint32>(cls.id);
            const int hue = static_cast<int>((seed * 2654435761u) % 360u); // spread IDs over hue wheel
            cls.color = QColor::fromHsv(hue, 200, 230);
        }
        
        cfg.classes_.push_back(cls);
    }

    // Load applyMeanStd (default to true if not present)
    cfg.applyMeanStd_ = obj.value("applyMeanStd").toBool(true);

    // Load normalizeTo01 (default to true if not present)
    cfg.normalizeTo01_ = obj.value("normalizeTo01").toBool(true);

    return cfg;
}

const std::vector<float>& ModelConfig::getMean() const noexcept {
    return mean_;
}

const std::vector<float>& ModelConfig::getStd() const noexcept {
    return std_;
}

const std::string& ModelConfig::getModelPath() const noexcept {
    return modelPath_;
}

uint ModelConfig::getInputAlignment() const noexcept {
    return inputAlignment_;
}

const std::vector<ModelConfig::ClassInfo>& ModelConfig::getClasses() const noexcept {
    return classes_;
}

const ModelConfig::ClassInfo* ModelConfig::getClassById(ClassId id) const noexcept {
    for (const auto& cls : classes_) {
        if (cls.id == id) {
            return &cls;
        }
    }
    return nullptr;
}

bool ModelConfig::setClassSelected(ClassId id, bool selected) noexcept {
    for (auto& cls : classes_) {
        if (cls.id == id) {
            cls.selected = selected;
            return true;
        }
    }
    return false;
}

bool ModelConfig::getApplyMeanStd() const noexcept {
    return applyMeanStd_;
}

bool ModelConfig::getNormalizeTo01() const noexcept {
    return normalizeTo01_;
}

void ModelConfig::setApplyMeanStd(bool apply) noexcept {
    applyMeanStd_ = apply;
}

void ModelConfig::setNormalizeTo01(bool normalize) noexcept {
    normalizeTo01_ = normalize;
}

void ModelConfig::setInputAlignment(uint alignment) noexcept {
    inputAlignment_ = alignment;
}