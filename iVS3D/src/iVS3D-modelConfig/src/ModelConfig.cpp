#include "ModelConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
        cfg.mean.push_back(static_cast<float>(val.toDouble()));
    }
    for (const auto& val : obj["std"].toArray()) {
        cfg.std.push_back(static_cast<float>(val.toDouble()));
    }

    // Load model path from config (may be empty)
    // Validation of model file existence is done elsewhere (e.g., ObjectDetectionModelManager)
    cfg.modelPath = obj["modelPath"].toString().toStdString();

    // Load resolution alignment
    if (obj.contains("resolutionAlignment")) {
        cfg.resolutionAlignment = static_cast<uint>(obj["resolutionAlignment"].toInt(1));
    }

    // Load input size (optional)
    if (obj.contains("inputSize")) {
        for (const auto& val : obj["inputSize"].toArray()) {
            cfg.inputSize.push_back(static_cast<uint>(val.toInt()));
        }
    }

    // Load class information
    uint id = 0;
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
        if (cls_obj.contains("id")) {
            cls.id = static_cast<uint>(cls_obj["id"].toInt());
        } else {
            cls.id = id++;
        }
        cfg.classes.push_back(cls);
    }

    return cfg;
}