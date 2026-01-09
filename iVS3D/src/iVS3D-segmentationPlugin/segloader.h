#pragma once

#include <QString>
#include <QVector>

#include <exception>
#include <optional>

#include "segmodel.h"

namespace segmentationplugin {

    class ModelLoader {
    public:
        ModelLoader(const QString& modelFolder);

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