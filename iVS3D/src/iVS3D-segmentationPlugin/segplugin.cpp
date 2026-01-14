#include "segplugin.h"

#include "NeuralUtil.h"
#include "QMessageBox"

std::unique_ptr<IMaskComputeSession>
SegmentationPlugin::createMaskComputeSession(
    const QMap<QString, QVariant>& settings) {
    if (settings.contains("modelName") && m_loader) {
        QString modelName = settings["modelName"].toString();
        auto info = m_loader->getModelByName(modelName);
        if (info) {
            return std::make_unique<SegmentationSession>(*info, m_useCuda);
        }
    }
    return nullptr;
}

VisualizationResult SegmentationPlugin::generatePreview(
    const PreviewData& data) {
    assert(m_settingsWidget);  // core should ensure to load settings widget
                               // before enabling preview

    if (!m_currentInfo) {
        return tl::make_unexpected(
            Error{ErrorCode::RuntimeError,
                  tr("No model selected for segmentation preview.")});
    }

    if (!m_currentSession) {
        m_currentSession =
            std::make_unique<SegmentationSession>(*m_currentInfo, m_useCuda);
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
        auto inferenceResult = m_currentSession->runInference(data.image);

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
    }

    if (m_cache->colorizedImage.empty()) {
        // Compute colorized image if not already done
        auto colorResult =
            m_currentSession->runColorization(m_cache->inferenceTensor);

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
        auto maskResult =
            m_currentSession->runMasking(m_cache->inferenceTensor);

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
