#pragma once

/**
 * @file OrtNeuralNet.h
 * @brief Header file for the OrtNeuralNet class, which implements the NeuralNet interface using ONNX Runtime.
 * It also handles the conversion between Tensor and ONNX Runtime's Ort::Value.
 */

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
    /**
     * @ingroup NeuralNet
     * @class OrtNeuralNet
     * @brief A class that implements the NeuralNet interface using ONNX Runtime.
     * 
     * @details
     * This class is responsible for loading an ONNX model, performing inference, and converting between Tensor and ONNX Runtime's Ort::Value.
     * It supports both CPU and GPU execution, depending on the model and the environment setup.
     * 
     * @note DO NOT use this class directly in your code. Instead, use the NeuralNetFactory and NeuralNet interface to interact with neural networks.
     * 
     * Usage:
     * @code{.cpp}
     * // Create an OrtNeuralNet instance
     * auto neuralNet = NeuralNetFactory::create("model.onnx");
     * if (!neuralNet) {
     *     std::cerr << "Failed to create neural network: " << neuralNet.error() << std::endl;
     *     return -1;
     * }
     * 
     * // Create a Tensor with the correct shape and data type
     * Shape inputShape = neuralNet->inputShape(); // assume it returns {1, 3, 224, 224} for a single image input
     * cv::Mat inputMat(224, 224, CV_32FC3, cv::Scalar(0.0f)); // Example input
     * Tensor inputTensor = Tensor::fromCvMat(inputMat).value(); // Convert cv::Mat to Tensor
     * 
     * // Perform inference
     * auto output = neuralNet->infer(inputTensor);
     * if (!output) {
     *     std::cerr << "Inference failed: " << output.error() << std::endl;
     *     return -1;
     * }
     * Tensor outputTensor = output.value();
     * @endcode
     */
    class OrtNeuralNet : public NeuralNet
    {
    public:
        /**
         * @brief Construct a new OrtNeuralNet object.
         * 
         * @param modelPath The path to the ONNX model file.
         * @param useCuda Whether to use CUDA for GPU execution. Default is false (CPU).
         * @param gpuId The ID of the GPU to use if CUDA is enabled. Default is 0.
         * 
         * @note DO NOT use this constructor directly in your code. Instead, use the NeuralNetFactory to create an instance of this class.
         */
        OrtNeuralNet(const std::string& modelPath, bool useCuda = false, int gpuId = 0);

        /**
         * @brief Perform inference on the given input Tensor using the ONNX model.
         * @param input The input Tensor to the neural network. This tensor must have the correct shape and data type expected by the model.
         * @return tl::expected<std::vector<Tensor>, NeuralError> The output Tensor or an error object.
         */
        tl::expected<std::vector<Tensor>, NeuralError> infer(const Tensor& input) override;

        /**
         * @brief Get the number of inputs of the neural network.
         * @return size_t The number of inputs.
         */
        size_t inputCount() const override;

        /**
         * @brief Get the number of outputs of the neural network.
         * @return size_t The number of outputs.
         */
        size_t outputCount() const override;

        /**
         * @brief Get the input shape of the neural network.
         * @param idx The index of the input to query (default is 0). Valid indices are in the range [0, inputCount()).
         * @return Shape The shape of the input tensor expected by the model.
         */
        Shape inputShape(size_t idx = 0) const override;
        
        /**
         * @brief Get the output shape of the neural network.
         * @param idx The index of the output to query (default is 0). Valid indices are in the range [0, outputCount()).
         * @return Shape The shape of the output tensor produced by the model.
         */
        Shape outputShape(size_t idx = 0) const override;

        /**
         * @brief Get the name of the input tensor.
         * @param idx The index of the input to query (default is 0). Valid indices are in the range [0, inputCount()).
         * @return std::string The name of the input tensor.
         */
        std::string inputName(size_t idx = 0) const override;

        /**
         * @brief Get the name of the output tensor.
         * @param idx The index of the output to query (default is 0). Valid indices are in the range [0, outputCount()).
         * @return std::string The name of the output tensor.
         */
        std::string outputName(size_t idx = 0) const override;
        /**
         * @brief Get the GPU ID used by the neural network if it is configured to use GPU.
         * @return int The GPU ID, or -1 if the neural network does not use GPU.
         */
        int gpuId() const override;

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_sessionOptions;
        Ort::Session m_session{ nullptr };
        Ort::AllocatorWithDefaultOptions m_allocator;

        std::vector<int64_t> m_inputShape; // for now only one input is supported!
        std::vector<std::vector<int64_t>> m_outputShapes;
        std::string m_inputName;
        std::vector<std::string> m_outputNames;
        int m_gpuId;

        tl::expected<Ort::Value, NeuralError> tensorToOrtValue(const Tensor& tensor, std::optional<std::vector<int64_t>> shapeOverride = std::nullopt) const;
        tl::expected<Tensor, NeuralError> ortValueToTensor(const Ort::Value& value) const;
    };
}

