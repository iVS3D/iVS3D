#pragma once

#include <variant>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>
#include <numeric>
#include <iostream>
#include <sstream>
#include <array>
#include <type_traits>
#include <utility>

#include <opencv2/core.hpp>

#include <tl/expected.hpp>

#include <ReduceOps.h>

/**
 * @brief NN Neural Network Library containing Tensor and NeuralNet classes for inference.
 * 
 * @details Contains the Tensor class for representing N-dimensional arrays with various data types.
 * The Tensor class supports operations like reshaping, mapping, reducing, and converting to/from cv::Mat.
 * 
 * A factory handles model loading and abstracts the underlying inference engine (e.g., ONNX Runtime).
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 * 
 */
namespace NN
{
    /**
     * @brief Shape of a N-dimensional Tensor represented as the size in each dimension. Can be -1 in case of dynamic dimensions.
     * 
     * @code 
     * NeuralNet net; 
     * net.inputShape(); // might be [-1,3,512,512] in NCHW format with a dynamic batch size, 3 channels and 512x512 pixels
     * @endcode
     *  */ 
    using Shape = std::vector<int64_t>;

    /**
     * @brief Calculates the number of elements from a given Shape.
     * 
     * @details Returns @code shape[0] * shape[1] * ... * shape[N-1] @endcode 
     * 
     * @param shape The shape object.
     * @return int64_t The number of elements in a Tensor with that shape.
     */
    int64_t shapeNumElements(const Shape& shape);

    /**
     * @brief Calculates the stride to iterate elements in a given axis.
     * 
     * @param shape The shape of the tensor to iterate.
     * @param axis The axis to iterate. This must be in range [0,shape.size()-1]!
     * @return int64_t the stride (stepsize) to iterate the given axis.
     */
    int64_t shapeToStride(const Shape& shape, uint64_t axis);

    /**
     * @brief Creates a human-readable string from the given shape.
     * 
     * @param shape The shape to convert.
     * @return std::string A string with the shapes elements in printable form.
     */
    std::string shapeToString(const Shape& shape);

    /**
     * @brief TensorType encapsulates the supported data types of tensor elements.
     * The supported types are:
     * - Float
     * - Int64
     * - UInt8
     * 
     */
    enum class TensorType {
        Float,       // float32
        Int64,       // int64_t
        UInt8,       // uint8_t
        Invalid      // Invalid type, used for error handling
    };

    /**
     * @brief Convert the TensorType to a human-readable string.
     * 
     * @param type The TensorType to convert.
     * @return constexpr const char* Returns a string representation of the type.
     */
    constexpr const char* toString(TensorType type) {
        switch (type) {
            case TensorType::Float: return "float32";
            case TensorType::Int64: return "int64";
            case TensorType::UInt8: return "uint8";
            default: return "Invalid";
        }
    };

    // --- Helper to extract std::array traits
    template <typename T>
    struct is_std_array : std::false_type {};

    template <typename U, std::size_t N>
    struct is_std_array<std::array<U, N>> : std::true_type {
        using value_type = U;
        static constexpr size_t size = N;
    };

    // Remove cv/ref qualifiers and decay to check std::array
    template <typename T>
    using decay_t = typename std::decay<T>::type;

    template <typename Func, typename InputElem>
    struct map_array_traits {
        using return_type = std::decay_t<std::invoke_result_t<Func, InputElem>>;
        static_assert(is_std_array<return_type>::value, "Function must return std::array");

        using value_type = typename is_std_array<return_type>::value_type;
        static constexpr size_t size = is_std_array<return_type>::size;
    };
    // --- End heplers
    
    /**
     * @brief A Tensor represents a N-dimensional array containing elements of the same type. Can be used as input and output for inference.
     * 
     * @details
     * Supported element types are:
     * 
     * - uint8_t
     * - float
     * - int64_t
     * 
     * @see TensorType for the supported types.
     * 
     * Tensors can be created from and converted to cv::Mat or std::vector<T>. 
     * The element type is passed as a Template argument or deduced from the cv::Mat::depth property. 
     * 
     * @date May 2025
     * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
     */
    class Tensor
    {
    public:
        using TensorData = std::variant<
            std::vector<float>,
            std::vector<int64_t>,
            std::vector<uint8_t>
        >;

        /**
         * @brief Create a new Tensor object from a cv::Mat. This will convert from CVs HWC layout to ONNX standard layout [N]CHW. 
         * In this case the N -dimsension is NOT created as it would be 1!
         * 
         * @param mat The cv::Mat containing the data in HWC layout for the Tensor.
         * @return tl::expected<Tensor, std::string> A Tensor in CHW layout or an error message.
         */
        static tl::expected<Tensor, std::string> fromCvMat(const cv::Mat& mat);

        /**
         * @brief Create a new Tensor object from a given data vector and shape. The number of elements in the vector must match 
         * @code shapeNumElements(shape) @endcode.
         * 
         * @tparam T The datatype, this must be on of the following: uint8_t, int64_t, float.
         * @param data The data vector. This will be copied!
         * @param shape The shape of the new Tensor.
         * @return tl::expected<Tensor, std::string> A Tensor object containing the data and shape or an error message.
         */
        template <typename T>
        static tl::expected<Tensor, std::string> fromData(const std::vector<T>& data, const Shape& shape) {
            return fromData(std::vector<T>(data), shape);  // delegates to move overload
        }

        /**
         * @brief Create a new Tensor object from a given data vector and shape. The number of elements in the vector must match 
         * @code shapeNumElements(shape) @endcode.
         * 
         * @tparam T The datatype, this must be on of the following: uint8_t, int64_t, float.
         * @param data The data vector. The new Tensor takes ownership of the data!
         * @param shape The shape of the new Tensor.
         * @return tl::expected<Tensor, std::string> A Tensor object containing the data and shape or an error message.
         */
        template <typename T>
        static tl::expected<Tensor, std::string> fromData(std::vector<T>&& data, const Shape& shape) {
            if (static_cast<size_t>(shapeNumElements(shape)) != data.size()) {
                return tl::unexpected("Data size does not match shape");
            }

            Tensor t;
            t.m_shape = shape;
            t.m_data = std::move(data);
            return t;
        }

        /**
         * @brief Create a vector containing the data from the Tensor.
         * 
         * @tparam T The datatype of the vector. This must match the Tensors current type!
         * @return tl::expected<std::vector<T>, std::string>  Retruns the data vector or an error message if the Tensor does not contain the requested data type.
         */
        template <typename T>
        tl::expected<std::vector<T>, std::string> toVector() const {
            if (std::holds_alternative<std::vector<T>>(m_data)) {
                return std::get<std::vector<T>>(m_data);
            }
            return tl::unexpected("Tensor does not hold requested data type");
        }

        /**
         * @brief Create a cv::Mat from a Tensor. In case of 2/3 dimensions this will convert back to CVs HWC layout. Returns an error for dimensions other than 2 or 3.
         * 
         * @return tl::expected<cv::Mat, std::string> A cv::Mat object with the data in HWC format or an error message.
         */
        tl::expected<cv::Mat, std::string> toCvMat() const;
        
        /**
         * @brief Readonly access to the shape of the tensor. Valid tensors ensure the shape is static, so no dimension is -1.
         * 
         * @return const Shape& access to the shape object.
         */
        const Shape& shape() const { return m_shape; }

        /**
         * @brief Create a human-readable string representation containing the Tensors shape and data type.
         * 
         * @return std::string A string like "Tensor(shape=[3,512,512], dtype=float)".
         */
        std::string toString() const;

        /**
         * @brief Reshape will interprete the data elements contained in the Tensor as a different shape. This does not move any elements!
         * 
         * @param newShape The new shape, it must match the Tensors number of elements.
         * @return tl::expected<void, std::string> Returns an error message only if the new shape is invalid.
         */
        tl::expected<void, std::string> reshape(const Shape& newShape);

        /**
         * @brief Returns the number of elements contained in the Tensor.
         * 
         * @return int64_t The number of elements.
         */
        int64_t numElements() const;

        /**
         * @brief Check if the tensor is empty, so to say it contains no elements.
         * 
         * @return true If the Tensor contains no elements.
         * @return false If the Tensor contains one or more element.
         */
        bool empty() const {
            return m_shape.empty() || shapeNumElements(m_shape) == 0;
        }

        /**
         * @brief Check the data type of the Tensor. This is deduced from the data type of 
         * the contained elements.
         * 
         * @return TensorType 
         */
        TensorType dtype() const {
            return std::visit([](const auto& vec) -> TensorType {
                using T = typename std::decay_t<decltype(vec)>::value_type;
                if constexpr (std::is_same_v<T, float>) {
                    return TensorType::Float;
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    return TensorType::Int64;
                } else if constexpr (std::is_same_v<T, uint8_t>) {
                    return TensorType::UInt8;
                } else {
                    return TensorType::Invalid; // Unsupported type
                }
            }, m_data);
        };

        /**
         * @brief Reduce the Tensor along a given axis by applying an accumulative operation.
         * 
         * @tparam Op Templated to work with different data types. See @see ReduceOps.h ReduceOps.h
         * @param op An accumulative operation like ReduceMin/ReduceMax/ReduceSum.
         * @param axis The axis in which we apply the operation, in the output this axis will have size 1.
         * @return tl::expected<Tensor, std::string> Returns a reduced Tensor or an error message.
         */
        template <typename Op>
        tl::expected<Tensor, std::string> reduce(const Op& op, uint64_t axis) const {
            if (axis >= m_shape.size()) {
                return tl::unexpected("Invalid reduction axis");
            }

            return std::visit([&](const auto& inputVec) -> tl::expected<Tensor, std::string> {
                using T = typename std::decay_t<decltype(inputVec)>::value_type;
                const int64_t D = m_shape.size();

                Shape outShape = m_shape;
                outShape[axis] = 1;

                std::vector<T> output(shapeNumElements(outShape));

                int64_t innerStride = shapeToStride(m_shape, axis);
                int64_t dimSize = m_shape[axis];
                int64_t outerStride = innerStride * dimSize;

                for (int64_t offset = 0; offset < inputVec.size(); offset += outerStride) {
                    for (int64_t i = 0; i < innerStride; ++i) {
                        // Compute flat index
                        int64_t outIdx = (offset / outerStride) * innerStride + i;
                        // Initialize accumulator
                        T acc = op.template initial<T>();
                        for (int64_t d = 0; d < dimSize; ++d) {
                            int64_t idx = offset + d * innerStride + i;
                            op(acc, inputVec[idx]); // Apply operation
                        }
                        output[outIdx] = acc;
                    }
                }
                return Tensor::fromData(std::move(output), outShape);
            }, m_data);
        }

        /**
         * @brief Reduce the Tensor along a given axis by applying an accumulative operation. 
         * The dimension in the rduced axis will be 1 after this.
         * 
         * @tparam Op Templated to work with different data types. See @see ReduceOps.h ReduceOps.h
         * @param op An indexed reduction operation like ReduceArgMax.
         * @param axis The axis in which we apply the operation, in the output this axis will have size 1.
         * @return tl::expected<Tensor, std::string> Returns a reduced Tensor or an error message.
         * 
         * @details Example usage for applying an ArgMax-Operation to a 2D-Tensor.
         * Assume the Tensor contains a batch of K-dimesnional feature vectors and batch size is N. 
         * Then the shape is [N,K]. We want to obtain the index of the maximum value for 
         * each vector, so the output will have shape [N,1]:
         * 
         * @code {.cpp}
         * NN::Tensor tensor = ... // shape is [N,K] 
         * auto result = tensor.reduceWithIndex(NN::ReduceArgMax{},1); // apply arg-max to axis 1 (corresponds to K)
         * if (!result){
         *   std::cout << "ERROR: " << result.error() << std::endl;
         *   return -1;
         * } 
         * NN::Tensor argmax_tensor = result.value();
         * std::cout << "Resulting tensor: " << argmax_tensor.toString() << std::endl; 
         * // Resulting tensor: tensor(shape=[N,1], dtype=int64)
         * @endcode
         * 
         */
        template <typename Op>
        tl::expected<Tensor, std::string> reduceWithIndex(const Op& op, uint64_t axis) const {
            if (axis >= m_shape.size()) {
                return tl::unexpected("Invalid reduction axis");
            }

            return std::visit([&](const auto& inputVec) -> tl::expected<Tensor, std::string> {
                using T = typename std::decay_t<decltype(inputVec)>::value_type;

                Shape outShape = m_shape;
                outShape[axis] = 1;

                std::vector<int64_t> output(shapeNumElements(outShape));

                int64_t innerStride = shapeToStride(m_shape, axis);
                int64_t dimSize = m_shape[axis];
                int64_t outerStride = innerStride * dimSize;

                for (int64_t offset = 0; offset < inputVec.size(); offset += outerStride) {
                    for (int64_t i = 0; i < innerStride; ++i) {
                        int64_t outIdx = (offset / outerStride) * innerStride + i;
                        auto acc = op.template initial<T>();
                        for (size_t d = 0; d < dimSize; ++d) {
                            size_t idx = offset + d * innerStride + i;
                            op(acc, inputVec[idx], d);
                        }
                        output[outIdx] = static_cast<int64_t>(acc.first);
                    }
                }

                return Tensor::fromData(std::move(output), outShape);
            }, m_data);
        }

        /**
         * @brief Map each element of the Tensor to a new value by applying a given function element-wise. 
         * This can be used to change the datatype as well.
         * 
         * @tparam Func Templated function type (T->V) where T matches the current data type and V is a valid new data type.
         * @param f The mapping function which is applied to each element.
         * @return tl::expected<Tensor, std::string> Returns a new Tensor or an error message.
         */
        template <typename Func>
        tl::expected<Tensor, std::string> map(Func&& f) const{
            return std::visit([&](const auto& inputVec) -> tl::expected<Tensor, std::string> {
                using T = typename std::decay_t<decltype(inputVec)>::value_type;
                using V = std::decay_t<decltype(f(std::declval<T>()))>;

                std::vector<V> output;
                output.resize(inputVec.size());

                std::transform(inputVec.begin(), inputVec.end(), output.begin(),
                    [&](const T& val) -> V {
                        return f(val);
                    });

                return Tensor::fromData(std::move(output), m_shape);

            }, m_data);
        }

        /**
         * @brief Map each element of the Tensor to an array of new values by applying a given function element-wise.
         * This adds a new dimension in the given axis.
         * 
         * @tparam Func Templated function type (T->std::array<N,U>) where T matches the current data type and U is a valid new data type.
         * @param func The mapping function which is applied to each element.
         * @param axis The axis to insert the new dimension.
         * @return tl::expected<Tensor, std::string> Returns a new Tensor or an error message.
         * 
         * @details Example usage: Map a 2D-Tensor of class-indices to a 3D-Tensor with RGB colors assigned to each class.decay_t
         * The mapping function therefore maps a class index (int64_t) to a RGB color (std::array<3,uint8_t>).
         * @code {.cpp}
         * NN::Tensor classes = ... // Tensor(shape=[512,512], dtype=int64)
         * std::vector<std::array<uint8_t,3>> colors = { {255,0,0}, {127,127,127}, ... }; // a color per class
         * auto result = classes.map([colors](int64_t index){ 
         *  return colors[index]; 
         * }, 0);
         * if (!result) {
         *  std::cerr << "ERROR: " << result.error() << std::endl;
         *  return -1;
         * }
         * NN::Tensor colorized = std::move(result.value());
         * std::cout << colorized.toString() << std::endl; // Tensor(shape=[3,512,512], dtype=uint8)
         * @endcode
         */
        template <typename Func>
        tl::expected<Tensor, std::string> map(Func&& func, int axis) const {
            // Sanity check axis
            if (axis < 0 || axis > static_cast<int>(m_shape.size())) {
                return tl::unexpected("Invalid axis to insert new dimension.");
            }
        
            return std::visit([&](const auto& input) -> tl::expected<Tensor, std::string> {
                using T = typename std::decay_t<decltype(input)>::value_type;
                using Traits = map_array_traits<Func, T>;
                using U = typename Traits::value_type;
                constexpr size_t N = Traits::size;
            
                if (input.empty()) {
                    return tl::unexpected("Tensor data is empty.");
                }
            
                // Compute the original and new shapes
                Shape newShape = m_shape;
                newShape.insert(newShape.begin() + axis, N);
            
                const int64_t oldSize = shapeNumElements(m_shape);
                const int64_t newSize = oldSize * N;
            
                std::vector<U> outData(newSize);
            
                // Precompute strides
                std::vector<int64_t> oldStrides = computeStrides(m_shape);
                std::vector<int64_t> newStrides = computeStrides(newShape);
            
                
                // Main loop: for each index in the old tensor, place mapped values in the new buffer
                for (int64_t idx = 0; idx < oldSize; ++idx) {
                    // Convert flat idx to N-dimensional index
                    std::vector<int64_t> coord = unravelIndex(idx, oldStrides);
                
                    // Apply function to get array<U, N>
                    auto mapped = func(input[idx]);
                
                    // Insert mapped[N] into new buffer at axis
                    coord.insert(coord.begin() + axis, 0);
                    for (int64_t i = 0; i < static_cast<int64_t>(N); ++i) {
                        coord[axis] = i;
                        int64_t flatIdx = ravelIndex(coord, newStrides);
                        outData[flatIdx] = mapped[i];
                    }
                }
            
                return Tensor::fromData(std::move(outData), newShape);
            }, m_data);
        }

        tl::expected<void, std::string> squeeze();

        tl::expected<void, std::string> squeeze(int64_t axis);

        tl::expected<void, std::string> unsqueeze(int64_t axis);


    private:
        TensorData m_data;
        Shape m_shape;
        
        static inline std::vector<int64_t> computeStrides(const Shape& shape) {
            std::vector<int64_t> strides(shape.size(), 1);
            for (int i = shape.size() - 2; i >= 0; --i)
                strides[i] = strides[i + 1] * shape[i + 1];
            return strides;
        }

        static inline std::vector<int64_t> unravelIndex(int64_t index, const std::vector<int64_t>& strides) {
            std::vector<int64_t> coords(strides.size());
            for (size_t i = 0; i < strides.size(); ++i) {
                coords[i] = index / strides[i];
                index %= strides[i];
            }
            return coords;
        }

        static inline int64_t ravelIndex(const std::vector<int64_t>& coords, const std::vector<int64_t>& strides) {
            int64_t index = 0;
            for (size_t i = 0; i < coords.size(); ++i)
                index += coords[i] * strides[i];
            return index;
        }

        friend class OrtNeuralNet;
    };
}

#ifndef TENSOR_DEBUG_PRINT
#ifdef NDEBUG
    #define TENSOR_DEBUG_PRINT(tensor) ((void)0);
#else
    #include <iostream>
    #define TENSOR_DEBUG_PRINT(tensor) (std::cout << (tensor).toString() << std::endl);
#endif
#endif

#ifndef SHAPE_DEBUG_PRINT
#ifdef NDEBUG
    #define SHAPE_DEBUG_PRINT(shape) ((void)0);
#else
    #include <iostream>
    #define SHAPE_DEBUG_PRINT(shape) (std::cout << NN::shapeToString(shape) << std::endl);
#endif
#endif

#ifndef RETURN_ON_ERROR
#define RETURN_ON_ERROR(expr, retVal)                               \
    do {                                                            \
        auto _res = (expr);                                         \
        if (!_res) {                                                \
            std::cerr << "Error: " << _res.error() << std::endl;    \
            return (retVal);                                        \
        }                                                           \
    } while (0);
#endif