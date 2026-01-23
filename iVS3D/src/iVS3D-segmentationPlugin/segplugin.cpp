#include "segplugin.h"

#include "NeuralUtil.h"
#include "QMessageBox"
#include <chrono>

SettingsWidgetResult SegmentationPlugin::getSettingsWidget(QWidget* parent) {
    if (!m_loader) {
        auto res = initializeModelLoader();
        if (res) {
            return tl::make_unexpected(res.value());
        }
    }

    if (!m_settingsWidget) {
        m_settingsWidget = std::make_shared<SettingsWidget>(
            parent, m_loader ? m_loader->getModels() : QVector<ModelInfo>());
        m_settingsWidget->setOverlayAlpha(0.5f);  // default alpha
        connect(m_settingsWidget.get(), &SettingsWidget::modelChanged, this,
                &SegmentationPlugin::onModelChanged);
        connect(m_settingsWidget.get(), &SettingsWidget::overlayAlphaChanged,
                [this](float alpha) {
                    m_overlayAlpha =
                        alpha;  // does not require cache invalidation
                    emit updatePreview(false);
                });
        connect(m_settingsWidget.get(), &SettingsWidget::classSelectionChanged,
                this, &SegmentationPlugin::onClassSelectionChanged);
    }

    return m_settingsWidget;
}

MaskResult SegmentationPlugin::generateMask(const MaskData& data) {
    assert(m_currentSession);  // core should ensure that the plugin is in a
                               // valid state before calling this

    auto inferenceResult = runInference(data.image);

    if (!inferenceResult) {
        return tl::make_unexpected(
            Error{ErrorCode::RuntimeError,
                  tr("An error occurred during model inference:\n%1")
                      .arg(QString::fromStdString(
                          inferenceResult.error().message()))});
    }

    auto maskResult = runMasking(m_cache->inferenceTensor);

    if (!maskResult) {
        return tl::make_unexpected(Error{
            ErrorCode::RuntimeError,
            tr("An error occurred during mask computation.\n%1")
                .arg(QString::fromStdString(maskResult.error().message()))});
    }

    return maskResult.value();
}

VisualizationResult SegmentationPlugin::generatePreview(
    const PreviewData& data) {
    assert(m_settingsWidget);  // core should ensure to load settings widget
                               // before enabling preview

    if (!m_currentSession) {
        return tl::make_unexpected(
            Error{ErrorCode::RuntimeError,
                  tr("No model selected for segmentation preview.")});
    }

    // Check cache validity
    bool useCache = m_cache && m_cache->idx == data.index &&
                    !m_cache->image.empty() &&
                    m_cache->image.cols == data.image.cols &&
                    m_cache->image.rows == data.image.rows;
    if (!useCache) {
        m_cache = std::nullopt;  // Invalidate cache if index does not match
    }

    if (!m_cache) {
        // Inference if we don't have a cached result
        auto start = std::chrono::high_resolution_clock::now();
        
        auto inferenceResult = runInference(data.image);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (!inferenceResult) {
            if (inferenceResult.error().code() == NN::ErrorCode::OutOfMemory) {
                return tl::make_unexpected(
                    Error{ErrorCode::RuntimeError,
                          tr("Ran out of memory during inference!\n"
                             "Lower the working resolution to reduce memory "
                             "usage.")});
            } else {
                return tl::make_unexpected(
                    Error{ErrorCode::RuntimeError,
                          tr("An error occurred during model inference:\n%1")
                              .arg(QString::fromStdString(
                                  inferenceResult.error().message()))});
            }
        }
        m_cache = SegmentationCache{data.index, data.image,
                                    std::move(inferenceResult.value())};
        m_cache->inferenceDurationMs = durationMs;
    } else {
        m_cache->inferenceDurationMs = 0;  // Cached result
    }

    if (m_cache->colorizedImage.empty()) {
        // Compute colorized image if not already done
        auto colorResult = runColorization(m_cache->inferenceTensor);

        if (!colorResult) {
            return tl::make_unexpected(
                Error{ErrorCode::RuntimeError,
                      tr("An error occurred during colorization.\n%1")
                          .arg(QString::fromStdString(
                              colorResult.error().message()))});
        }
        m_cache->colorizedImage = std::move(colorResult.value());
        // resize the result to the original width and height
        if (m_cache->colorizedImage.cols != data.image.cols ||
            m_cache->colorizedImage.rows != data.image.rows) {
            cv::resize(m_cache->colorizedImage, m_cache->colorizedImage,
                       cv::Size(data.image.cols, data.image.rows), 0, 0,
                       cv::INTER_NEAREST);
        }
    }

    if (m_cache->maskImage.empty()) {
        // Compute mask image if not already done
        auto maskResult = runMasking(m_cache->inferenceTensor);

        if (!maskResult) {
            return tl::make_unexpected(Error{
                ErrorCode::RuntimeError,
                tr("An error occurred during mask computation.\n%1")
                    .arg(
                        QString::fromStdString(maskResult.error().message()))});
        }
        m_cache->maskImage = std::move(maskResult.value());
        // resize the result to the original width and height
        if (m_cache->maskImage.cols != data.image.cols ||
            m_cache->maskImage.rows != data.image.rows) {
            cv::resize(m_cache->maskImage, m_cache->maskImage,
                       cv::Size(data.image.cols, data.image.rows), 0, 0,
                       cv::INTER_NEAREST);
        }
    }

    Visualization vis;
    {
        auto& view = vis.views.emplace_back();
        if (m_cache->inferenceDurationMs > 0) {
            view.title = tr("Segmentation Preview (inference time: %1 ms)").arg(m_cache->inferenceDurationMs);
        } else {
            view.title = tr("Segmentation Preview (cached)");
        }
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::FullImage;
        view.style.showTitle = true;

        ImageOverlay overlay;
        overlay.image = m_cache->colorizedImage;
        overlay.style.opacity = m_overlayAlpha;

        view.overlays.push_back(overlay);
    }
    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Segmentation Mask");
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::FullImage;
        view.style.showTitle = true;

        ImageOverlay overlay;
        overlay.image = m_cache->maskImage;
        overlay.style.opacity = 1.0f;
        view.overlays.push_back(overlay);
    }

    return vis;
}

std::optional<Error> SegmentationPlugin::initializeModelLoader() {
    try {
        m_loader = std::make_shared<ModelLoader>(m_modelFolder);

    } catch (const std::exception& e) {
        return Error{ErrorCode::ResourceUnavailable,
                     tr("Failed to initialize model loader:\n%1")
                         .arg(QString::fromStdString(e.what()))};
    }
    if (m_loader->getModels().empty()) {
        return Error{ErrorCode::ResourceUnavailable,
                     tr("No segmentation models found in folder:\n%1")
                         .arg(m_modelFolder)};
    }
    m_currentSession = {m_loader->getModels().front(), nullptr};
    return std::nullopt;
}

tl::expected<NN::Tensor, NN::NeuralError> SegmentationPlugin::runInference(
    const cv::Mat& image) {
    if (!m_currentSession->model) {
        auto result = NN::NeuralNetFactory::create(
            m_currentSession->info.path.toStdString(), m_useCuda);
        if (!result) {
            return tl::make_unexpected(result.error());
        }
        m_currentSession->model = *result;
    }
    return NN::Tensor::fromCvMat(image, m_currentSession->model->inputShape(),
                                 1.0f, m_currentSession->info.mean,
                                 m_currentSession->info.std)
        .and_then(NN::Util::bind_inference(m_currentSession->model))
        .and_then([](NN::Tensor&& tensor)
                      -> tl::expected<NN::Tensor, NN::NeuralError> {
            // squeeze the tensor to remove leading dimensions of size 1
            if (tensor.dtype() == NN::TensorType::Float) {
                return tensor.reduceWithIndex(NN::ReduceArgMax{}, 1);
            }
            return tl::expected<NN::Tensor, NN::NeuralError>(std::move(tensor));
        })
        .and_then(NN::Util::bind_squeeze());
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationPlugin::runColorization(
    const NN::Tensor& inferenceTensor) {
    return inferenceTensor
        .map(
            [this](const int64_t& value) -> std::array<uint8_t, 3> {
                return {
                    static_cast<uint8_t>(
                        m_currentSession->info.classes[value].color.red()),
                    static_cast<uint8_t>(
                        m_currentSession->info.classes[value].color.green()),
                    static_cast<uint8_t>(
                        m_currentSession->info.classes[value].color.blue())};
            },
            0)
        .and_then(NN::Util::bind_toCvMat());
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationPlugin::runMasking(
    const NN::Tensor& inferenceTensor) {
    return inferenceTensor
        .map([classes = m_currentSession->info.classes](
                 const int64_t& value) -> uint8_t {
            return classes[value].selected ? 255
                                           : 0;  // only keep selected classes
        })
        .and_then(NN::Util::bind_toCvMat());
}
