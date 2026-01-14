#include "segsession.h"

#include <NeuralNetFactory.h>
#include <NeuralUtil.h>

using namespace segmentationplugin;

segmentationplugin::SegmentationSession::SegmentationSession(
    const ModelInfo& modelInfo, bool useCuda)
    : m_info(modelInfo), m_model(nullptr), m_useCuda(useCuda) {}

MaskResult segmentationplugin::SegmentationSession::generateMask(
    const MaskData& data) {
    return MaskResult();
}

tl::expected<NN::Tensor, NN::NeuralError> SegmentationSession::runInference(
    const cv::Mat& image) {
    if (!m_model) {
        auto result =
            NN::NeuralNetFactory::create(m_info.path.toStdString(), m_useCuda);
        if (!result) {
            return tl::make_unexpected(result.error());
        }
        m_model = *result;
    }
    return NN::Tensor::fromCvMat(image, m_model->inputShape(), 1.0f,
                                 m_info.mean, m_info.std)
        .and_then(NN::Util::bind_inference(m_model))
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

tl::expected<cv::Mat, NN::NeuralError> SegmentationSession::runColorization(
    const NN::Tensor& inferenceTensor) {
    return inferenceTensor
        .map(
            [this](const int64_t& value) -> std::array<uint8_t, 3> {
                return {
                    static_cast<uint8_t>(m_info.classes[value].color.red()),
                    static_cast<uint8_t>(m_info.classes[value].color.green()),
                    static_cast<uint8_t>(m_info.classes[value].color.blue())};
            },
            0)
        .and_then(NN::Util::bind_toCvMat());
}

tl::expected<cv::Mat, NN::NeuralError> SegmentationSession::runMasking(
    const NN::Tensor& inferenceTensor) {
    return inferenceTensor
        .map([classes = m_info.classes](const int64_t& value) -> uint8_t {
            return classes[value].selected ? 255
                                           : 0;  // only keep selected classes
        })
        .and_then(NN::Util::bind_toCvMat());
}