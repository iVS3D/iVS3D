#pragma once

#include <NeuralNet.h>
#include <exception>
#include "imask.h"
#include "segmodel.h"

namespace segmentationplugin {

class SegmentationSession : public IMaskComputeSession {
   public:
    SegmentationSession(const ModelInfo& modelInfo, bool useCuda);
    ~SegmentationSession() override = default;

    MaskResult generateMask(const MaskData& data) override;

    tl::expected<NN::Tensor, NN::NeuralError> runInference(const cv::Mat& image);
    tl::expected<cv::Mat, NN::NeuralError> runColorization(const NN::Tensor& inferenceTensor);
    tl::expected<cv::Mat, NN::NeuralError> runMasking(const NN::Tensor& inferenceTensor);

    void setModelInfo(const ModelInfo& modelInfo) { m_info = modelInfo; }

   private:
    ModelInfo m_info;
    NN::NeuralNetPtr m_model;
    bool m_useCuda;
};

}  // namespace segmentationplugin