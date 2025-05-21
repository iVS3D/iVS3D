#pragma once

#include "Tensor.h"

#include <tl/expected.hpp>
#include <string>
#include <vector>
#include <memory>

namespace NN
{
    class NeuralNet {
    public:
        virtual ~NeuralNet() = default;

        // Perform inference given an input tensor
        virtual tl::expected<Tensor, std::string> infer(const Tensor& input) = 0;

        // Operator overload for syntactic sugar: model(input)
        tl::expected<Tensor, std::string> operator()(const Tensor& input) {
            return infer(input);
        }

        // Query the input shape expected by the model
        virtual std::vector<int64_t> inputShape() const = 0;

        // Query the output shape produced by the model
        virtual std::vector<int64_t> outputShape() const = 0;
    };

    // Smart pointer alias
    using NeuralNetPtr = std::shared_ptr<NeuralNet>;
}