#pragma once

#include <NeuralNetFactory.h>

#include <QCoreApplication>
#include <QLabel>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"
#include "segloader.h"
#include "settingswidget.h"

using namespace segmentationplugin;

class SegmentationPlugin : public IBase, public IMask, public IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IMask")
    Q_PLUGIN_METADATA(IID "iVS3D.IPreview")
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(IMask IBase IPreview)

   public:
    using IBase::IBase;

    QString getName() const override {
        return tr("Segmentation Plugin with Preview");
    }

    SettingsWidgetResult getSettingsWidget(QWidget* parent) override;

    QMap<QString, QVariant> getSettings() const override {
        QMap<QString, QVariant> settings;
        if (m_currentSession) {
            settings["modelName"] = m_currentSession->info.name;

            QStringList selectedClasses;
            for (const auto& cls : m_currentSession->info.classes) {
                if (cls.selected) {
                    selectedClasses.append(cls.name);
                }
            }
            settings["selectedClasses"] = selectedClasses;
        }
        settings["overlayAlpha"] = m_overlayAlpha;
        return settings;
    }

    ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override {
        // find the model name in the given settings
        if (!settings.contains("modelName"))
            return tl::make_unexpected(Error{ErrorCode::InvalidInput,
                         tr("modelName is required in settings.")});
        QString modelName = settings["modelName"].toString();

        // make sure we can load models
        if (!m_loader) {
            auto initError = initializeModelLoader();
            if (initError) {
                return tl::make_unexpected(initError.value());
            }
        }

        // load the model
        auto model = m_loader->getModelByName(modelName);
        if (!model) {
            return tl::make_unexpected(Error{ErrorCode::InvalidInput,
                         tr("Model '%1' not found.").arg(modelName)});
        }
        m_currentSession = {model.value(), nullptr};
        m_cache = std::nullopt;  // Clear cache

        // load selected classes
        for (auto& classInfo : m_currentSession->info.classes)
            classInfo.selected = false;

        if (settings.contains("selectedClasses")) {
            QStringList selectedClasses =
                settings["selectedClasses"].toStringList();
            for (auto& classInfo : m_currentSession->info.classes) {
                if (selectedClasses.contains(classInfo.name)) {
                    classInfo.selected = true;
                }
            }
        }

        // load overlay alpha
        if (settings.contains("overlayAlpha")) {
            m_overlayAlpha = settings["overlayAlpha"].toFloat();
        }

        // update settings widget if it exists
        if (m_settingsWidget) {
            m_settingsWidget->setModel(m_currentSession->info);
            m_settingsWidget->setOverlayAlpha(m_overlayAlpha);
        }
        return {};
    }

    MaskResult generateMask(const MaskData& data) override;

    VisualizationResult generatePreview(const PreviewData& data) override;

    void onCudaChanged(bool enabled) override {
        m_useCuda = enabled;
        if (m_currentSession) {
            m_currentSession->model =
                nullptr;             // force reload with new cuda setting
            m_cache = std::nullopt;  // Clear cache
            emit updatePreview(true);
        }
    }

   private slots:
    void onModelChanged(const QString& modelName) {
        m_currentSession = std::nullopt;  // force reload with new model
        m_cache = std::nullopt;           // Clear entire cache on model change

        if (auto modelInfo = m_loader->getModelByName(modelName)) {
            m_currentSession = ModelSession{*modelInfo, nullptr};
            m_settingsWidget->setModel(m_currentSession->info);
            // Do something with the selected model
        } else {
            emit encounteredError(
                Error{ErrorCode::InvalidInput,
                      tr("Model '%1' could not be found.").arg(modelName)});
            return;
        }

        emit updatePreview();
    }

    void onClassSelectionChanged(const QVector<bool>& selectedClasses) {
        if (m_cache) {
            m_cache->maskImage =
                cv::Mat();  // Invalidate only mask image in cache
        }  // Segmentation and colorized image can remain
        assert(selectedClasses.size() == m_currentSession->info.classes.size());
        for (size_t i = 0; i < selectedClasses.size(); ++i)
            m_currentSession->info.classes[i].selected = selectedClasses[i];

        emit updatePreview(false);
    }

   private:
    tl::expected<NN::Tensor, NN::NeuralError> runInference(
        const cv::Mat& image);
    tl::expected<cv::Mat, NN::NeuralError> runColorization(
        const NN::Tensor& inferenceTensor);
    tl::expected<cv::Mat, NN::NeuralError> runMasking(
        const NN::Tensor& inferenceTensor);
    std::optional<Error> initializeModelLoader();

    QString m_modelFolder = QCoreApplication::applicationDirPath() +
                            "/plugins/resources/neural_network_models";

    std::shared_ptr<ModelLoader> m_loader;
    std::shared_ptr<SettingsWidget> m_settingsWidget;

    bool m_useCuda = false;
    float m_overlayAlpha = 0.5f;

    struct ModelSession {
        ModelInfo info;
        NN::NeuralNetPtr model;
    };
    std::optional<ModelSession> m_currentSession;

    // cache for the last inference result (if any and valid)
    struct SegmentationCache {
        uint idx;
        cv::Mat image;
        NN::Tensor inferenceTensor;
        cv::Mat colorizedImage =
            cv::Mat();                  // optional: can be empty if invalid
        cv::Mat maskImage = cv::Mat();  // optional: can be empty if invalid
        long inferenceDurationMs = 0;   // Time taken for inference
    };
    std::optional<SegmentationCache> m_cache;
};