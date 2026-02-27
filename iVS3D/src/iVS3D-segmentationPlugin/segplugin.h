#pragma once

#include <ModelManager.h>
#include <NeuralNetFactory.h>

#include <QCoreApplication>
#include <QObject>
#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"

class SegmentationSettingsWidget;

class SegmentationPlugin : public IBase, public IMask, public IPreview {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IMask")
    Q_PLUGIN_METADATA(IID "iVS3D.IPreview")
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(IMask IBase IPreview)

   public:
    using IBase::IBase;

    SegmentationPlugin();

    // IBase interface
    QString getName() const override { return tr("Segmentation Masks"); }
    SettingsWidgetResult getSettingsWidget() override;
    QMap<QString, QVariant> getSettings() const override;
    QString getSettingsString() const override;
    ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override;
    void onCudaChanged(bool enabled) override;
    void deactivate() override;

    // IMask interface
    MaskResult generateMask(const MaskData& data) override;

    // IPreview interface
    VisualizationResult generatePreview(const PreviewData& data) override;

   signals:
    void syncSettingsWidget(QString selectedModelName, float overlayAlpha);

   private slots:
    void onModelChanged(const QString& modelName, bool isValid);
    void onClassSelectionChanged(const QVector<uint>& selectedClassIds);
    void onOverlayAlphaChanged(float value);

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