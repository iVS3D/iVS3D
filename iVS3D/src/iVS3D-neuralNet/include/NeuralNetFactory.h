#pragma once

/**
 * @file NeuralNetFactory.h
 * @brief Factory class for creating NeuralNet instances.
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include "NeuralNet.h"

#include <memory>
#include <string>
#include <iostream>
#include <tl/expected.hpp>

namespace NN
{
    /**
     * @ingroup NeuralNet
     * @class NeuralNetFactory
     * @brief Factory class for creating NeuralNet instances. This abstracts from the underlying inference engine (e.g., ONNX Runtime).
     */
    class NeuralNetFactory
    {
    public:
        /**
         * @brief Create a NeuralNet instance from a model file.
         * 
         * @param modelPath The path to the model file (e.g., ONNX model).
         * @param useCuda Whether to use CUDA for inference (default: false).
         * @param gpuId The GPU ID to use if CUDA is enabled (default: 0).
         * @return tl::expected<NeuralNetPtr, std::string> A pointer to the created NeuralNet or an error message.
         * 
         * @details
         * This method loads the model from the specified path and creates an instance of NeuralNet.
         * If the model cannot be loaded or the creation fails, an error message is returned.
         */
        static tl::expected<NeuralNetPtr, std::string> create(const std::string& modelPath, bool useCuda = false, int gpuId = 0);
    };
}