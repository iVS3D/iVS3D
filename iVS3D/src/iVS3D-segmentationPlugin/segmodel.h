#pragma once

#include <QColor>
#include <QString>
#include <QVector>

#include <exception>
#include <optional>

namespace segmentationplugin {

    struct ModelClass {
        QString name;
        QColor color;
        bool selected = false;
    };

    struct ModelInfo {
        QString name;
        QString path;
        std::vector<float> mean, std;
        QVector<ModelClass> classes;
    };
}