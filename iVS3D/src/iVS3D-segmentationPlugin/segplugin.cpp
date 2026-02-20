#include "segplugin.h"

#include <chrono>

#include "NeuralUtil.h"
#include "QMessageBox"

std::optional<Error> SegmentationPlugin::ensureModelReady() {
    auto activeModel = m_modelManager.activeModel();
    if (!activeModel || !activeModel->config) {
        return Error{ErrorCode::ResourceUnavailable,
                     tr("No segmentation model is currently active. "
                        "Please select a model in the plugin settings.")};
    }

    // Verify model name matches (in case model was changed)
    if (m_currentModelName != activeModel->name) {
        m_currentModel = nullptr;  // Force reload if model changed
        m_currentModelName = activeModel->name;
    }

    // Load neural network if not already loaded
    if (!m_currentModel) {
        auto result = NN::NeuralNetFactory::create(
            activeModel->onnxPath.toStdString(), m_useCuda);
        if (!result) {
            return Error{
                ErrorCode::RuntimeError,
                tr("Failed to load neural network model: %1")
                    .arg(QString::fromStdString(result.error().message()))};
        }
        m_currentModel = *result;
    }

    return std::nullopt;
}

SegmentationPlugin::SegmentationPlugin() {
    m_overlayAlpha = 0.5f;  // Default overlay alpha
    m_modelManager.setNameFilter("Segmentation_*");
}

SettingsWidgetResult SegmentationPlugin::getSettingsWidget(QWidget* parent) {
    if (!m_settingsWidget) {
        // Create container widget
        m_settingsWidget = std::make_shared<QWidget>(parent);
        auto* mainLayout = new QVBoxLayout(m_settingsWidget.get());
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(8, 8, 8, 8);

        // Add ModelSettingsWidget
        m_modelSettingsWidget =
            new ModelSettingsWidget(m_modelManager, m_settingsWidget.get());
        mainLayout->addWidget(m_modelSettingsWidget);

        // Add separator
        auto* separator = new QFrame();
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(separator);

        // Add Overlay Alpha slider
        auto* alphaLayout = new QHBoxLayout();
        auto* alphaLabel = new QLabel(tr("Overlay Opacity:"));
        m_alphaSlider = new QSlider(Qt::Horizontal);
        m_alphaSlider->setRange(0, 100);
        m_alphaSlider->setValue(static_cast<int>(m_overlayAlpha * 100));
        m_alphaSlider->setToolTip(
            tr("Adjust transparency of the segmentation overlay (0% = "
               "transparent, 100% = opaque)"));
        m_alphaValue = new QLabel(QString::number(m_overlayAlpha, 'f', 2));
        m_alphaValue->setMinimumWidth(40);
        m_alphaValue->setAlignment(Qt::AlignCenter);
        alphaLayout->addWidget(alphaLabel);
        alphaLayout->addWidget(m_alphaSlider, 1);
        alphaLayout->addWidget(m_alphaValue);
        mainLayout->addLayout(alphaLayout);

        mainLayout->addStretch();

        // Connect signals from ModelSettingsWidget (model/class selection)
        connect(m_modelSettingsWidget, &ModelSettingsWidget::modelChanged, this,
                [this](const QString& modelName, bool isValid) {
                    if (isValid) {
                        onModelChanged(modelName);
                    } else {
                        m_cache = std::nullopt;
                        emit updatePreview(true);
                    }
                });
        connect(m_modelSettingsWidget,
                &ModelSettingsWidget::classSelectionChanged, this,
                [this](const QVector<uint>& selectedIds) {
                    onClassSelectionChanged(selectedIds);
                });
        connect(m_modelSettingsWidget,
                &ModelSettingsWidget::modelConfigChanged, this,
                [this]() {
                    // need to recalculate everything if config changes
                    m_cache = std::nullopt;
                    emit updatePreview(true);
                });

        // Connect alpha slider
        connect(m_alphaSlider, &QSlider::valueChanged, [this](int value) {
            m_overlayAlpha = static_cast<float>(value) / 100.0f;
            m_alphaValue->setText(QString::number(m_overlayAlpha, 'f', 2));
            emit updatePreview(false);  // No need to re-run inference
        });
    }

    return m_settingsWidget;
}

MaskResult SegmentationPlugin::generateMask(const MaskData& data) {
    // Ensure model is ready before processing
    if (auto error = ensureModelReady()) {
        return tl::make_unexpected(*error);
    }

    auto inferenceResult = runInference(data.image);

    if (!inferenceResult) {
        return tl::make_unexpected(
            Error{ErrorCode::RuntimeError,
                  tr("An error occurred during model inference:\n%1")
                      .arg(QString::fromStdString(
                          inferenceResult.error().message()))});
    }

    auto maskResult = runMasking(inferenceResult.value());

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

    // Ensure model is ready before processing
    if (auto error = ensureModelReady()) {
        return tl::make_unexpected(*error);
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
        auto durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();

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
            view.title = tr("Segmentation Preview (inference time: %1 ms)")
                             .arg(m_cache->inferenceDurationMs);
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

tl::expected<NN::Tensor, NN::NeuralError> SegmentationPlugin::runInference(
    const cv::Mat& image) {
    if (auto error = ensureModelReady()) {
        return tl::make_unexpected(NN::NeuralError{
            NN::ErrorCode::RuntimeError, error->message.toStdString()});
    }

    const auto& config = m_modelManager.activeModel()->config;
    return NN::Tensor::fromCvMat(
               image, m_currentModel->inputShape(),
               config->getNormalizeTo01() ? 1.0f / 255.0f : 1.0f,
               config->getApplyMeanStd() ? config->getMean() : std::vector<float>{}, 
               config->getApplyMeanStd() ? config->getStd() : std::vector<float>{})
        .and_then(NN::Util::bind_inference(m_currentModel))
        .and_then(NN::Util::bind_selectOutput(0))
        .and_then([](NN::Tensor&& tensor)
                      -> tl::expected<NN::Tensor, NN::NeuralError> {
            // Tensor is expected to be either Float (logits) or Int64 (classId)
            // If Float, reduce to ArgMax along channel dimension [NCHW] ->
            // [NHW]
            if (tensor.dtype() == NN::TensorType::Float) {
                return tensor.reduceWithIndex(NN::ReduceArgMax{}, 1);
            }
            // Now its Int64 with a classId per pixel as expected
            return tl::expected<NN::Tensor, NN::NeuralError>(std::move(tensor));
        })
        .and_then(NN::Util::bind_squeeze());  // remove leading singleton dims
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationPlugin::runColorization(
    const NN::Tensor& inferenceTensor) {
    // Model is guaranteed to be ready by ensureModelReady()
    auto activeModel = m_modelManager.activeModel();
    const auto& classes = activeModel->config->getClasses();

    return inferenceTensor
        .map(
            [&classes](const int64_t& value) -> std::array<uint8_t, 3> {
                if (value >= 0 &&
                    value < static_cast<int64_t>(classes.size())) {
                    const auto& color = classes[value].color;
                    return {static_cast<uint8_t>(color.red()),
                            static_cast<uint8_t>(color.green()),
                            static_cast<uint8_t>(color.blue())};
                }
                return {0, 0, 0};
            },
            0)
        .and_then(NN::Util::bind_toCvMat());
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationPlugin::runMasking(
    const NN::Tensor& inferenceTensor) {
    // Model is guaranteed to be ready by ensureModelReady()
    auto activeModel = m_modelManager.activeModel();
    const auto& classes = activeModel->config->getClasses();

    return inferenceTensor
        .map([&classes](const int64_t& value) -> uint8_t {
            if (value >= 0 && value < static_cast<int64_t>(classes.size())) {
                return classes[value].selected ? 255 : 0;
            }
            return 0;
        })
        .and_then(NN::Util::bind_toCvMat());
}
