#pragma once

#include "NeuralNet.h"

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <any>
#include <iostream>
#include <optional>

#include <onnxruntime_cxx_api.h>

namespace NN
{
    class OrtNeuralNet : public NeuralNet
    {
    public:
        OrtNeuralNet(const std::string& modelPath, bool useCuda = false, int gpuId = 0);

        tl::expected<Tensor, std::string> infer(const Tensor& input) override;
        std::vector<int64_t> inputShape() const override;
        std::vector<int64_t> outputShape() const override;

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_sessionOptions;
        Ort::Session m_session{ nullptr };
        Ort::AllocatorWithDefaultOptions m_allocator;

        std::vector<int64_t> m_inputShape;
        std::vector<int64_t> m_outputShape;
        std::string m_inputName;
        std::string m_outputName;
        int m_gpuId;

        tl::expected<Ort::Value, std::string> tensorToOrtValue(const Tensor& tensor, std::optional<std::vector<int64_t>> shapeOverride = std::nullopt) const;
        tl::expected<Tensor, std::string> ortValueToTensor(const Ort::Value& value) const;
    };
}

