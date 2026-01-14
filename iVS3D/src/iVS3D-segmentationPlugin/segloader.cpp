#include "segloader.h"

#include <QDir>
#include <QFileInfoList>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace segmentationplugin;



ModelLoader::ModelLoader(const QString& modelFolder) {
    // here we'll load the models from the specified folder
    if (modelFolder.isEmpty()) {
        throw std::invalid_argument("Model folder path is empty.");
    }

    if (!QDir(modelFolder).exists()) {
        throw std::invalid_argument("Model folder does not exist: " + modelFolder.toStdString());
    }

    QDir dir(modelFolder);
    // get all .onnx files in the directory
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "Segmentation_*.onnx", QDir::Files);

    for (const QFileInfo& fileInfo : fileList) {
        ModelInfo model;
        model.name = fileInfo.baseName();
        model.path = fileInfo.absoluteFilePath();

        // find associated .json config file
        QString configPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".json";
        if (!QFile::exists(configPath)) {
            printf("Config file does not exist for model: %s\n", model.name.toStdString().c_str());
            continue;
        }

        QFile configFile(configPath);
        if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            printf("Failed to open config file: %s\n", configPath.toStdString().c_str());
            continue;
        }
        QByteArray configData = configFile.readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(configData, &err);
        if (err.error != QJsonParseError::NoError) {
            printf("Failed to parse config file: %s Error: %s\n", configPath.toStdString().c_str(), err.errorString().toStdString().c_str());
            continue;
        }

        QJsonObject obj = doc.object();

        // Load mean and std vectors
        for (auto mean_val : obj["mean"].toArray()) {
            model.mean.push_back(static_cast<float>(mean_val.toDouble()));
        }
        for (auto std_val : obj["std"].toArray()) {
            model.std.push_back(static_cast<float>(std_val.toDouble()));
        }

        // load class names and colors
        for (auto classObj : obj["classes"].toArray()) {
            QJsonObject classJson = classObj.toObject();
            ModelClass modelClass;
            modelClass.name = classJson["name"].toString();
            auto colorVal = classJson["color"].toArray();
            if (colorVal.size() != 3) {
                printf("Invalid color format in config for model: %s\n", model.name.toStdString().c_str());
                continue;
            }
            modelClass.color = QColor(colorVal[0].toInt(), colorVal[1].toInt(), colorVal[2].toInt());
            model.classes.append(modelClass);
        }

        m_models.append(model);
    }
}