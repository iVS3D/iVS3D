#pragma once

/**
 * @file NeuralNet.h
 * @brief Contains the NeuralNet interface for neural network inference.
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include "Tensor.h"

#include <tl/expected.hpp>
#include <string>
#include <vector>
#include <memory>

namespace NN
{
    /**
     * @class NeuralNet
     * @brief Abstract base class for neural networks.
     * 
     * @details
     * This class defines the interface for neural networks, including methods for inference,
     * input/output shape queries, and operator overloading for convenience.
     * 
     * @see NeuralNetPtr for a smart pointer alias to this class.
     */
    class NeuralNet {
    public:
        virtual ~NeuralNet() = default;

        /**
         * @brief Perform inference on the given input tensor.
         * 
         * @param input The input tensor to the neural network. This tensor must have the correct shape and data type expected by the model.
         * @return tl::expected<Tensor, std::string> The output tensor or an error message.
         * 
         * @details
         * This method takes an input tensor, processes it through the neural network, and returns the output tensor.
         * The input tensor must match the expected input shape of the model.
         * If the input tensor is invalid or the inference fails, an error message is returned.
         */
        virtual tl::expected<Tensor, std::string> infer(const Tensor& input) = 0;

        /**
         * @brief Call the infer method with the given input tensor.
         * 
         * @param input The input tensor to the neural network.
         * @return tl::expected<Tensor, std::string> The output tensor or an error message.
         * 
         * @see infer(const Tensor& input) for more details.
         */
        tl::expected<Tensor, std::string> operator()(const Tensor& input) {
            return infer(input);
        }

        /**
         * @brief Get the input shape of the neural network. This might contain dynamic dimensions (e.g., -1 for batch size).
         * 
         * @return Shape The input shape of the neural network.
         */
        virtual Shape inputShape() const = 0;

        /**
         * @brief Get the output shape of the neural network. This might contain dynamic dimensions (e.g., -1 for batch size).
         * 
         * @return Shape The output shape of the neural network.
         */
        virtual Shape outputShape() const = 0;
    };

    /**
     * @brief Smart pointer type for managing NeuralNet instances.
     */
    using NeuralNetPtr = std::shared_ptr<NeuralNet>;
}