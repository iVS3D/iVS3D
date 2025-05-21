#pragma once

#include <variant>
#include <vector>
#include <cstdint>
#include <string>
#include <cmath>
#include <numeric>
#include <iostream>
#include <sstream>

#include <opencv2/core.hpp>

#include <tl/expected.hpp>

namespace NN
{
    int64_t shapeNumElements(const std::vector<int64_t>& shape);
    std::string shapeToString(const std::vector<int64_t>& shape);

    /// @brief 
    class Tensor
    {
    public:
        using TensorData = std::variant<
            std::vector<float>,
            std::vector<int64_t>,
            std::vector<uint8_t>
        >;

        static tl::expected<Tensor, std::string> fromCvMat(const cv::Mat& mat);

        template <typename T>
        static tl::expected<Tensor, std::string> fromData(const std::vector<T>& data, const std::vector<int64_t>& shape) {
            if (data.size() != shapeNumElements(shape)) {
                return tl::unexpected("Data size does not match shape");
            }

            Tensor t;
            t.m_shape = shape;
            t.m_data = data;
            return t;
        }

        template <typename T>
        tl::expected<std::vector<T>, std::string> toVector() const {
            if (std::holds_alternative<std::vector<T>>(m_data)) {
                return std::get<std::vector<T>>(m_data);
            }
            return tl::unexpected("Tensor does not hold requested data type");
        }

        tl::expected<cv::Mat, std::string> toCvMat() const;
        
        const std::vector<int64_t>& shape() const { return m_shape; }

        std::string toString() const;

        tl::expected<void, std::string> reshape(const std::vector<int64_t>& newShape);

        int64_t numElements() const;

    private:
        TensorData m_data;
        std::vector<int64_t> m_shape;

        Tensor();

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