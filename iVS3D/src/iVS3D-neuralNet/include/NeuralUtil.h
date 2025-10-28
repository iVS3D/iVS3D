#pragma once

/**
 * @file NeuralUtil.h
 * @defgroup NeuralUtil NeuralUtil
 * @ingroup NeuralNet
 * 
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
 * @ingroup NeuralUtil
 * @brief Namespace for utility functions related to neural networks and tensors.
 * 
 * @details
 * This namespace contains functions to bind tensor operations such as reduction, mapping, reshaping, squeezing, unsqueezing, and converting to OpenCV Mat format.
 * These functions are designed to be used with the Tensor class and can be easily integrated into neural network workflows.
 * The functions return callable objects that can be used with tl::expected::and_then for chaining operations.
 */
namespace NN::Util {

/**
 * @ingroup NeuralUtil
 * @brief Binds a reduction operation on a specified axis for a tensor.
 * 
 * @param op The reduction operation to apply, such as ReduceSum, ReduceMin, ReduceMax.
 * @param axis The axis along which to perform the reduction.
 * 
 * @see Tensor::reduce for the main reduction function that applies this operation.
 */
template<typename ReduceOp>
auto bind_reduce(ReduceOp op, int axis) {
    return [=](NN::Tensor&& t){
        return t.reduce(op, axis);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a reduction operation with index on a specified axis for a tensor.
 * 
 * @param op The reduction operation with index to apply, such as ReduceArgMin, ReduceArgMax.
 * @param axis The axis along which to perform the reduction.
 * 
 * @see Tensor::reduceWithIndex for the main reduction function that applies this operation.
 */
template<typename ReduceIndexOp>
auto bind_reduceWithIndex(ReduceIndexOp op, int axis) {
    return [=](NN::Tensor&& t) {
        return t.reduceWithIndex(op, axis);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a mapping operation on a tensor.
 * 
 * @param f The function to apply to each element of the tensor.
 * 
 * @details This function allows you to apply a function to each element of the tensor.
 * It can be used to transform the data in the tensor, such as normalizing values or converting types.
 * 
 * @see Tensor::map for the main mapping function that applies this operation.
 */
template<typename Func>
auto bind_map(Func f) {
    return [=](NN::Tensor&& t) {
        return t.map(f);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a mapping operation on a tensor that converts a each element to an array of new elements.
 * 
 * @param f The function to apply to each element of the tensor.
 * @param axis where to insert the new dimension from the array.
 * 
 * @details This function allows you to apply a function to each element of the tensor and convert it to an array of new elements.
 * It can be used to transform the data in the tensor, such as mapping each element to a color.
 * 
 * @see Tensor::map for the main mapping function that applies this operation.
 */
template<typename Func>
auto bind_map(Func f, int axis) {
    return [=](NN::Tensor&& t) {
        return t.map(f, axis);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a reshape operation to change the shape of a tensor.
 * 
 * @param newShape The new shape to reshape the tensor to.
 * 
 * @details This function allows you to reshape a tensor to a new shape specified by the newShape vector.
 * The new shape must be compatible with the number of elements in the tensor.
 * 
 * @see Tensor::reshape for the main reshape function that applies this operation.
 */
template<typename... Args>
auto bind_reshape(const std::vector<int64_t>& newShape) {
    return [=](NN::Tensor&& t) {
        return t.reshape(newShape);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a squeeze operation to remove dimensions of size 1 from a tensor.
 * 
 * @details This function allows you to remove dimensions of size 1 from the tensor, effectively reducing its dimensionality.
 * It can be used to simplify the shape of the tensor after operations that may have added unnecessary dimensions.
 * 
 * @see Tensor::squeeze for the main squeeze function that applies this operation.
 */
template<typename... Args>
auto bind_squeeze() {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor, NN::NeuralError> {
        auto result = tensor.squeeze();
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a squeeze operation on a tensor along a specified axis.
 * @param axis The axis to squeeze. This must be in the range [0, shape.size()-1] and the dimension at this axis must be 1.
 * 
 * @details This function allows you to remove a specific dimension of size 1 from the tensor.
 * It can be used to simplify the shape of the tensor after operations that may have added unnecessary dimensions.
 * If the specified axis is invalid or the dimension at that axis is not 1, an error message will be returned.
 * 
 * @see Tensor::squeeze for the main squeeze function that applies this operation.
 */
template<typename... Args>
auto bind_squeeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,NN::NeuralError> {
        auto result = tensor.squeeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds an unsqueeze operation to add a new dimension of size 1 at a specified axis in a tensor.
 * 
 * @param axis The axis to insert the new dimension. This must be in the range [0, shape.size()].
 * 
 * @details This function allows you to add a new dimension of size 1 at the specified axis in the tensor.
 * It can be used to prepare the tensor for operations that require a specific dimensionality.
 * If the specified axis is invalid, an error message will be returned.
 * 
 * @see Tensor::unsqueeze for the main unsqueeze function that applies this operation.
 */
template<typename... Args>
auto bind_unsqueeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,NN::NeuralError> {
        auto result = tensor.unsqueeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a conversion operation from a tensor to an OpenCV Mat format.
 * 
 * @see Tensor::toCvMat for the main conversion function that applies this operation.
 */
template<typename... Args>
auto bind_toCvMat() {
    return [=](NN::Tensor&& tensor) {
        return tensor.toCvMat();
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds a conversion operation from a tensor to a vector.
 * 
 * @see Tensor::toVector for the main conversion function that applies this operation.
 */
template<typename T, typename... Args>
auto bind_toVector() {
    return [=](NN::Tensor&& tensor) {
        return tensor.toVector<T>();
    };
}

/**
 * @ingroup NeuralUtil
 * @brief Binds an inference operation on a neural network model.
 * 
 * @param model The neural network model to perform inference on.
 * 
 * @details This function allows you to perform inference on a given input tensor using the specified neural network model.
 * It returns the output tensor after processing the input through the model.
 * 
 * @see NeuralNet::infer for the main inference function that applies this operation.
 */
template<typename... Args>
auto bind_inference(NN::NeuralNetPtr model) {
    return [=](NN::Tensor&& input) {
        return model->infer(input);
    };
}

} // namespace NN::Util