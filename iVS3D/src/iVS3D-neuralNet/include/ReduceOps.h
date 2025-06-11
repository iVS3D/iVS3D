#pragma once

/** @file ReduceOps.h
 *  @brief Contains reduction operations for tensors such as sum, min, max, argmin, and argmax.
 *  @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 *  @date May 2025
 * 
 *  @defgroup ReduceOps ReduceOps
 *  @ingroup NeuralNet
 */

#include <limits>
#include <cstddef>
#include <utility>

namespace NN {

/**
 * @ingroup ReduceOps
 * @brief Reduces a Tensor by summing its elements along a specified axis.
 * 
 * @details
 * This operation initializes an accumulator to zero and adds each element along the specified axis.
 * @code{.cpp}
 * Tensor tensor = ...; // some tensor
 * auto result = tensor.reduce(ReduceSum{}, 1); // reduce along axis 1
 * if (result) {
 *     std::cout << "Reduced Tensor: " << result.value().toString() << std::endl;
 * } else {
 *     std::cerr << "Error: " << result.error() << std::endl;
 * }
 * @endcode
 * 
 * @see Tensor::reduce For the main reduction function that applies this operation.
 * @see reduce.cpp For an example of using this operation.
 */
struct ReduceSum {
    template<typename T>
    T initial() const { return T(0); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        acc += value;
    }
};

/**
 * @ingroup ReduceOps
 * @brief Reduces a Tensor by computing the minimum value along a specified axis.
 * 
 * @details
 * This operation initializes an accumulator to the maximum possible value and updates it with the minimum value found along the specified axis.
 * 
 * @see Tensor::reduce For the main reduction function that applies this operation.
 * @see reduce.cpp For an example of using Tensor::reduce.
 */
struct ReduceMin {
    template<typename T>
    T initial() const { return std::numeric_limits<T>::max(); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        if (value < acc) acc = value;
    }
};

/**
 * @ingroup ReduceOps
 * @brief Reduces a Tensor by computing the maximum value along a specified axis.
 * 
 * @details
 * This operation initializes an accumulator to the lowest possible value and updates it with the maximum value found along the specified axis.
 * 
 * @see Tensor::reduce For the main reduction function that applies this operation.
 * @see reduce.cpp For an example of using Tensor::reduce.
 */
struct ReduceMax {
    template<typename T>
    T initial() const { return std::numeric_limits<T>::lowest(); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        if (value > acc) acc = value;
    }
};

/**
 * @ingroup ReduceOps
 * @brief Reduces a Tensor by finding the index of the minimum value along a specified axis.
 * 
 * @details
 * This operation initializes an accumulator with the first index and the maximum possible value.
 * It updates the accumulator with the index and value of the minimum element found along the specified axis.
 * 
 * @see Tensor::reduceWithIndex For the main reduction function that applies this operation.
 * @see reduceWithIndex.cpp For an example of using Tensor::reduceWithIndex.
 */
struct ReduceArgMin {
    template<typename T>
    std::pair<int64_t, T> initial() const {
        return {0, std::numeric_limits<T>::max()};
    }

    template<typename T>
    void operator()(std::pair<int64_t, T>& acc, const T& value, int64_t idx) const {
        if (value < acc.second) {
            acc = {idx, value};
        }
    }
};

/**
 * @ingroup ReduceOps
 * @brief Reduces a Tensor by finding the index of the maximum value along a specified axis.
 * 
 * @details
 * This operation initializes an accumulator with the first index and the lowest possible value.
 * It updates the accumulator with the index and value of the maximum element found along the specified axis.
 * 
 * @see Tensor::reduceWithIndex For the main reduction function that applies this operation.
 * @see reduceWithIndex.cpp For an example of using Tensor::reduceWithIndex.
 */
struct ReduceArgMax {
    template<typename T>
    std::pair<int64_t, T> initial() const {
        return {0, std::numeric_limits<T>::lowest()};
    }

    template<typename T>
    void operator()(std::pair<int64_t, T>& acc, const T& value, int64_t idx) const {
        if (value > acc.second) {
            acc = {idx, value};
        }
    }
};
}