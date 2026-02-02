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

    // Load input shape (optional)
    if (obj.contains("inputShape")) {
        for (const auto& val : obj["inputShape"].toArray()) {
            cfg.inputShape_.push_back(static_cast<uint>(val.toInt()));
        }
    }

    // Load class information and validate for duplicate IDs
    QSet<ClassId> seenIds;
    ModelConfig::ClassId autoId = 0;
    
    for (const auto& cls_val : obj["classes"].toArray()) {
        QJsonObject cls_obj = cls_val.toObject();
        ModelConfig::ClassInfo cls;
        cls.name = cls_obj["name"].toString();

        // Load color for this class
        auto c = cls_obj["color"].toArray();
        if (c.size() == 3) {
            cls.color = QColor(c[0].toInt(), c[1].toInt(), c[2].toInt());
        } else {
            // Default color if not specified
            cls.color = QColor(128, 128, 128);
        }
        
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
                      QString("Duplicate class ID found: %1").arg(cls.id)});
        }
        seenIds.insert(cls.id);
        
        cfg.classes_.push_back(cls);
    }

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

const std::vector<uint>& ModelConfig::getInputShape() const noexcept {
    return inputShape_;
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