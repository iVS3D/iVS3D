#pragma once

#include <QColor>
#include <QDir>
#include <QMap>
#include <QString>
#include <string>
#include <vector>

struct ModelConfig {
    std::vector<float> mean;
    std::vector<float> std;
    struct ClassInfo {
        QString name;
        QColor color;
        bool selected = false;
    };
    std::vector<ClassInfo> classes;
    std::string modelPath;
};

QMap<QString, ModelConfig> loadModels(QDir modelDir);