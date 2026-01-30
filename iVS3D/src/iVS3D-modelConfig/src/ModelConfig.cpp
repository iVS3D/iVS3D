#include "ModelConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QMap<QString, ModelConfig> loadModels(QDir modelDir) {
    if (!modelDir.exists()) return {};
    QMap<QString, ModelConfig> models;

    // iterate all *.onnx files in the directory
    QStringList nameFilters = {"Detection_*.onnx"};
    QFileInfoList fileInfoList =
        modelDir.entryInfoList(nameFilters, QDir::Files);
    for (const QFileInfo& fileInfo : fileInfoList) {
        QString modelName = fileInfo.baseName();
        ModelConfig modelInfo;
        modelInfo.modelPath = fileInfo.absoluteFilePath().toStdString();

        // Check if there is a corresponding .json file for metadata
        QString configPath =
            fileInfo.absolutePath() + "/" + modelName + ".json";
        if (!QFile::exists(configPath)) {
            std::printf(
                "[DetectionSampling] Warning: No JSON configuration found for "
                "model %s\n",
                modelName.toStdString().c_str());
            continue;
        }

        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::printf(
                "[DetectionSampling] Warning: Failed to open config file %s\n",
                configPath.toStdString().c_str());
            continue;
        }

        QByteArray data = file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError) {
            std::printf(
                "[DetectionSampling] Warning: JSON parse error in file %s: "
                "%s\n",
                configPath.toStdString().c_str(),
                err.errorString().toStdString().c_str());
            continue;
        }

        QJsonObject obj = doc.object();

        // Load normalization parameters
        for (const auto& val : obj["mean"].toArray()) {
            modelInfo.mean.push_back(static_cast<float>(val.toDouble()));
        }
        for (const auto& val : obj["std"].toArray()) {
            modelInfo.std.push_back(static_cast<float>(val.toDouble()));
        }

        // Load class information
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
            modelInfo.classes.push_back(cls);
        }

        models.insert(modelName, modelInfo);
    }
    return models;
}