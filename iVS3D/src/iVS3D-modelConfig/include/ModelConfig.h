#pragma once

#include <QColor>
#include <QDir>
#include <QMap>
#include <QString>
#include <string>
#include <vector>

#include <tl/expected.hpp>



struct ModelConfig {

    enum class ErrorCode {
        InvalidArgument = 0,
        IoError = 1,
        ConfigParseError = 2
    };

    struct Error {
        ErrorCode code;
        QString message;
    };

    std::vector<float> mean;
    std::vector<float> std;
    struct ClassInfo {
        uint id;
        QString name;
        QColor color;
        bool selected = false;
    };
    std::vector<ClassInfo> classes;
    std::string modelPath;

    uint resolutionAlignment = 1;
    std::vector<uint> inputSize;  // Optional: [width, height] for models with dynamic input

    static tl::expected<ModelConfig, ModelConfig::Error> loadFromFile(const QString& jsonPath);
};