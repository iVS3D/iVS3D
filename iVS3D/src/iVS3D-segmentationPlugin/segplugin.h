#pragma once

#include <ModelManager.h>
#include <ModelSettingsWidget.h>
#include <NeuralNetFactory.h>

#include <QCoreApplication>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>
#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"

class SegmentationPlugin : public IBase, public IMask, public IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IMask")
    Q_PLUGIN_METADATA(IID "iVS3D.IPreview")
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(IMask IBase IPreview)

   public:
    using IBase::IBase;

    SegmentationPlugin();

    QString getName() const override { return tr("Segmentation Masks"); }

    SettingsWidgetResult getSettingsWidget(QWidget* parent) override;

    QMap<QString, QVariant> getSettings() const override {
        QMap<QString, QVariant> settings;
        auto activeModel = m_modelManager.activeModel();
        if (activeModel && activeModel->config) {
            auto modelJson = m_modelManager.modelToJson(activeModel->name);
            settings.insert("selectedModel", modelJson);
        }
        settings["overlayAlpha"] = m_overlayAlpha;
        return settings;
    }

    QString getSettingsString() const override {
        return m_modelManager.modelToString(
            m_modelManager.activeModelName());
    }

    ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override {
        // find the model name in the given settings
        if (!settings.contains("selectedModel")) {
            return tl::make_unexpected(
                Error{ErrorCode::InvalidInput,
                      tr("selectedModel is required in settings.")});
        }
        QJsonObject modelObj = settings["selectedModel"].toJsonObject();
        auto modelEntryOpt = m_modelManager.modelFromJson(modelObj);
        if (!modelEntryOpt) {
            return tl::make_unexpected(
                Error{ErrorCode::InvalidInput,
                      tr("Failed to parse selected model from settings.")});
        }
        m_modelManager.activateModel(modelEntryOpt->name);
        // update settings widget if it exists
        if (m_modelSettingsWidget) {
            m_modelSettingsWidget->setSelectedModel(modelEntryOpt->name);
        }
        // load overlay alpha
        if (settings.contains("overlayAlpha")) {
            m_overlayAlpha = settings["overlayAlpha"].toFloat();
            // Update UI slider if it exists
            if (m_alphaSlider) {
                m_alphaSlider->setValue(
                    static_cast<int>(m_overlayAlpha * 100.0f));
            }
        }

        return {};
    }

    MaskResult generateMask(const MaskData& data) override;

    VisualizationResult generatePreview(const PreviewData& data) override;

    void onCudaChanged(bool enabled) override {
        m_useCuda = enabled;
        // Force reload of model with new CUDA setting
        m_cache = std::nullopt;  // Clear cache
        m_currentModelName.clear();
        m_currentModel = nullptr;
        emit updatePreview(true);
    }

    void deactivate() override {
        m_cache = std::nullopt;
        m_currentModelName.clear();
        m_currentModel = nullptr;
    }

   private slots:
    void onModelChanged(const QString& modelName) {
        m_currentModelName.clear();
        m_currentModel = nullptr;
        auto result = m_modelManager.activateModel(modelName);
        if (!result.has_value()) {
            emit encounteredError(
                Error{ErrorCode::InvalidInput,
                      tr("Model '%1' could not be found or activated.")
                          .arg(modelName)});
            return;
        }
        m_currentModelName = modelName;
        m_cache = std::nullopt;  // Clear entire cache on model change
        emit updatePreview(true);
    }

    void onClassSelectionChanged(const QVector<uint>& selectedClassIds) {
        if (m_cache) {
            m_cache->maskImage =
                cv::Mat();  // Invalidate only mask image in cache
        }
        emit updatePreview(false);
    }

   private:
    /**
     * @brief Ensures the model manager has an active, ready model and loads the
     * neural network. Validates that there is an active model in Ready state
     * and loads the neural network if needed.
     * @return std::nullopt if successful, Error if validation fails
     */
    std::optional<Error> ensureModelReady();

    tl::expected<NN::Tensor, NN::NeuralError> runInference(
        const cv::Mat& image);
    tl::expected<cv::Mat, NN::NeuralError> runColorization(
        const NN::Tensor& inferenceTensor);
    tl::expected<cv::Mat, NN::NeuralError> runMasking(
        const NN::Tensor& inferenceTensor);

    ModelManager m_modelManager{QCoreApplication::applicationDirPath() +
                                "/plugins/resources/neural_network_models"};

    // Settings widget is a container with ModelSettingsWidget + alpha slider
    std::shared_ptr<QWidget> m_settingsWidget;
    ModelSettingsWidget* m_modelSettingsWidget = nullptr;
    QSlider* m_alphaSlider = nullptr;
    QLabel* m_alphaValue = nullptr;

    bool m_useCuda = false;
    float m_overlayAlpha = 0.5f;

    QString m_currentModelName;
    NN::NeuralNetPtr m_currentModel;

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