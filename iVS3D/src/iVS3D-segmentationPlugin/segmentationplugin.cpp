#include "segmentationplugin.h"

#include "NeuralUtil.h"
#include "QMessageBox"

MaskResult SegmentationPlugin::generateMask(const MaskData& data) {
    // TO generate an export mask, we use the same logic as for the preview,
    // but errors are reported via return values instead of message boxes.
    // also, we do not use caching here.
    
    auto loadingError = ensureModelLoaded();
    if (loadingError) {
        return tl::make_unexpected(
            Error{ErrorCode::RuntimeError, *loadingError});
    }

    // IMPORTANT: In contrast to preview generation, we must respect the working
    // resolution specified in the MaskData! This may differ from the image
    // resolution and since the resolution affects the segmentation result, we
    // use the same resolution as for the preview to ensure consist results.
    cv::Mat imageToProcess;
    data.workingResolution.resize(data.image, imageToProcess);
    data.roi.crop(imageToProcess);

    auto inferenceResult = runInference(imageToProcess);
    if (!inferenceResult) {
        return tl::make_unexpected(Error{
            ErrorCode::RuntimeError,
            QString::fromStdString(inferenceResult.error().message())});
    }

    auto maskResult = runMasking(inferenceResult.value());
    if (!maskResult) {
        return tl::make_unexpected(Error{
            ErrorCode::RuntimeError,
            QString::fromStdString(maskResult.error().message())});
    }

    // No resizing needed here, the core will resize the mask to export resolution.
    return maskResult.value();
}

Visualization SegmentationPlugin::generatePreview(const PreviewData& data) {
    assert(m_settingsWidget);  // core should ensure to load settings widget
                               // before enabling preview

    // Ensure model is loaded
    if (auto loadingError = ensureModelLoaded()) {
        QMessageBox::warning(
            m_settingsWidget.get(), tr("Error"),
            tr("Failed to load model:\n%1").arg(*loadingError));
        return Visualization{};
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
        auto inferenceResult = runInference(data.image);

        if (!inferenceResult) {
            if (inferenceResult.error().code() == NN::ErrorCode::OutOfMemory) {
                QMessageBox::warning(
                    m_settingsWidget.get(), tr("Error"),
                    tr("Ran out of memory during inference!\n"
                       "Lower the working resolution to reduce memory usage."));
            } else {
                QMessageBox::warning(
                    m_settingsWidget.get(), tr("Error"),
                    tr("An error occurred during model inference:\n%1")
                        .arg(QString::fromStdString(
                            inferenceResult.error().message())));
            }
            return Visualization{};
        }
        m_cache = SegmentationCache{data.index, data.image,
                                    std::move(inferenceResult.value())};
    }

    if (m_cache->colorizedImage.empty()) {
        // Compute colorized image if not already done
        auto colorResult = runColorization(m_cache->inferenceTensor);

        if (!colorResult) {
            QMessageBox::warning(
                m_settingsWidget.get(), tr("Error"),
                tr("An error occurred during color mapping.\n%1")
                    .arg(
                        QString::fromStdString(colorResult.error().message())));
            return Visualization{};
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
            QMessageBox::warning(
                m_settingsWidget.get(), tr("Error"),
                tr("An error occurred during mask computation.\n%1")
                    .arg(QString::fromStdString(maskResult.error().message())));
            return Visualization{};
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
        view.title = tr("Segmentation Preview");
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

std::optional<QString> SegmentationPlugin::ensureModelLoaded() {
    if (!m_currentModel) {
        return tr("No model selected.");
    }
    if (!m_currentModel->model) {
        auto result = NN::NeuralNetFactory::create(
            m_currentModel->info.path.toStdString(), m_useCuda);
        if (!result) {
            return tr("Failed to load model.");
        }
        m_currentModel->model = *result;
    }

    auto inShape = m_currentModel->model->inputShape();
    if (inShape.size() < 2) {
        m_currentModel->model = nullptr;
        return tr("Failed to load model: Invalid input shape %1")
            .arg(QString::fromStdString(NN::shapeToString(inShape)));
    }
    return std::nullopt;
}

tl::expected<NN::Tensor, NN::NeuralError> SegmentationPlugin::runInference(
    const cv::Mat& image) {
    return NN::Tensor::fromCvMat(image, m_currentModel->model->inputShape(),
                                 1.0f, m_currentModel->info.mean,
                                 m_currentModel->info.std)
        .and_then(NN::Util::bind_inference(m_currentModel->model))
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
                return {static_cast<uint8_t>(
                            m_currentModel->info.classes[value].color.red()),
                        static_cast<uint8_t>(
                            m_currentModel->info.classes[value].color.green()),
                        static_cast<uint8_t>(
                            m_currentModel->info.classes[value].color.blue())};
            },
            0)
        .and_then(NN::Util::bind_toCvMat());
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationPlugin::runMasking(
    const NN::Tensor& inferenceTensor) {
    return inferenceTensor
        .map([classes = m_currentModel->info.classes](
                 const int64_t& value) -> uint8_t {
            return classes[value].selected ? 255
                                           : 0;  // only keep selected classes
        })
        .and_then(NN::Util::bind_toCvMat());
}
