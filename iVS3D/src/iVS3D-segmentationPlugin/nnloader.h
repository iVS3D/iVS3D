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

    class NNLoader {
    public:
        NNLoader(const QString& modelFolder);

        const QVector<ModelInfo>& getModels() const { return m_models; }
        std::optional<ModelInfo> getModelByName(const QString& name) const {
            for (const auto& model : m_models) {
                if (model.name == name) {
                    return model;
                }
            }
            return std::nullopt;
        }

    private:
        QVector<ModelInfo> m_models;
    };
}