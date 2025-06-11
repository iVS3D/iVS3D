#pragma once

/**
 * @file NeuralUtil.h
 * @brief Contains utility functions for tensor operations in neural networks.
 * 
 * @details This functional header provides utility functions to bind tensor operations such as reduction, mapping, reshaping, squeezing, unsqueezing, and converting to OpenCV Mat format.
 * These functions are designed to be used with the Tensor class and can be easily integrated into neural network workflows by using tl::expected::and_then for chaining operations.
 * 
 * @code{.cpp}
 * NeuralNetPtr model = ...; // some neural network model for semantic segmentation
 * cv::Mat image = ...;      // some OpenCV RGB input image
 * std::vector<std::array<uint8_t,3>> colors = {
 *    {0, 0, 0},    // background
 *    {255, 0, 0},  // class 1
 *    ...           // other classes
 * };
 * 
 * auto workflow = Tensor::fromCvMat(image)
 *    .and_then(Util::bind_map([](uint8_t val){
 *        return static_cast<float>(val) / 255.0f; // normalize to [0,1]
 *     })
 *    .and_then(Util::bind_inference(model))
 *    .and_then(Util::bind_reduce(ReduceArgMax, 1))
 *    .and_then(Util::bind_squeeze())
 *    .and_then(Util::bind_map([colors](int64_t val) {
 *        return colors[val]; // map class index to RGB color
 *     }, 0))
 *    .and_then(Util::bind_toCvMat());
 * 
 * if (!workflow) {
 *    std::cerr << "Error occurred during workflow: " << workflow.error() << std::endl;
 * }
 * cv::Mat outputImage = workflow.value();
 * @endcode
 *
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include <Tensor.h>
#include <NeuralNet.h>

/**
 * @brief Namespace for utility functions related to neural networks and tensors.
 * 
 * @details
 * This namespace contains functions to bind tensor operations such as reduction, mapping, reshaping, squeezing, unsqueezing, and converting to OpenCV Mat format.
 * These functions are designed to be used with the Tensor class and can be easily integrated into neural network workflows.
 * The functions return callable objects that can be used with tl::expected::and_then for chaining operations.
 */
namespace NN::Util {

template<typename ReduceOp>
auto bind_reduce(ReduceOp op, int axis) {
    return [=](NN::Tensor&& t){
        return t.reduce(op, axis);
    };
}

template<typename ReduceIndexOp>
auto bind_reduceWithIndex(ReduceIndexOp op, int axis) {
    return [=](NN::Tensor&& t) {
        return t.reduceWithIndex(op, axis);
    };
}

template<typename Func>
auto bind_map(Func f) {
    return [=](NN::Tensor&& t) {
        return t.map(f);
    };
}

template<typename Func>
auto bind_map(Func f, int axis) {
    return [=](NN::Tensor&& t) {
        return t.map(f, axis);
    };
}

template<typename... Args>
auto bind_reshape(const std::vector<int64_t>& newShape) {
    return [=](NN::Tensor&& t) {
        return t.reshape(newShape);
    };
}

template<typename... Args>
auto bind_squeeze() {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.squeeze();
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}


template<typename... Args>
auto bind_squeeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.squeeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

template<typename... Args>
auto bind_unsqueeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.unsqueeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

template<typename... Args>
auto bind_toCvMat() {
    return [=](NN::Tensor&& tensor) {
        return tensor.toCvMat();
    };
}


template<typename... Args>
auto bind_inference(NN::NeuralNetPtr model) {
    return [=](NN::Tensor&& input) {
        return model->infer(input);
    };
}

} // namespace NN::Util