#include "Tensor.h"

NN::Tensor::Tensor() 
{

}

tl::expected<NN::Tensor, std::string> NN::Tensor::fromCvMat(const cv::Mat& mat) {
    if (mat.empty()) {
        return tl::unexpected("cv::Mat is empty");
    }

    int channels = mat.channels();
    int height = mat.rows;
    int width = mat.cols;

    std::vector<int64_t> shape = { channels, height, width };

    if (mat.depth() == CV_8U) {
        std::vector<uint8_t> chw_data;
        chw_data.reserve(channels * height * width);

        std::vector<cv::Mat> splitted;
        cv::split(mat, splitted);

        for (int c = 0; c < channels; ++c) {
            chw_data.insert(chw_data.end(), splitted[c].datastart, splitted[c].dataend);
        }

        return Tensor::fromData(std::move(chw_data), shape);
    }

    if (mat.depth() == CV_32F) {
        std::vector<float> chw_data;
        chw_data.reserve(channels * height * width);

        std::vector<cv::Mat> splitted;
        cv::split(mat, splitted);

        for (int c = 0; c < channels; ++c) {
            const float* channelData = splitted[c].ptr<float>();
            chw_data.insert(chw_data.end(), channelData, channelData + height * width);
        }

        return Tensor::fromData(std::move(chw_data), shape);
    }

    return tl::unexpected("Unsupported cv::Mat type (only CV_8U and CV_32F are supported)");
}

std::string NN::Tensor::toString() const {

    std::ostringstream oss;
    oss << "Tensor(shape=[";
    for (size_t i = 0; i < m_shape.size(); ++i) {
        oss << m_shape[i];
        if (i + 1 < m_shape.size()) oss << ", ";
    }
    oss << "], dtype=";

    std::visit([&oss](auto&& vec) {
        using T = typename std::decay<decltype(vec)>::type::value_type;
        if constexpr (std::is_same_v<T, float>) {
            oss << "float32";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            oss << "int64";
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            oss << "uint8";
        } else {
            oss << "unknown";
        }
    }, m_data);
    oss << ")";
    return oss.str();
}

tl::expected<cv::Mat, std::string> NN::Tensor::toCvMat() const {
    int rows = m_shape.size() > 0 ? static_cast<int>(m_shape[0]) : 0;
    int cols = m_shape.size() > 1 ? static_cast<int>(m_shape[1]) : 1;
    int channels = m_shape.size() > 2 ? static_cast<int>(m_shape[2]) : 1;

    if (std::holds_alternative<std::vector<float>>(m_data)) {
        const auto& data = std::get<std::vector<float>>(m_data);
        if (data.size() != static_cast<size_t>(rows * cols * channels)) {
            return tl::unexpected("Data size does not match shape");
        }
        cv::Mat mat(rows, cols, CV_MAKETYPE(CV_32F, channels));
        std::memcpy(mat.data, data.data(), data.size() * sizeof(float));
        return mat;
    } else if (std::holds_alternative<std::vector<uint8_t>>(m_data)) {
        const auto& data = std::get<std::vector<uint8_t>>(m_data);
        if (data.size() != static_cast<size_t>(rows * cols * channels)) {
            return tl::unexpected("Data size does not match shape");
        }
        cv::Mat mat(rows, cols, CV_MAKETYPE(CV_8U, channels));
        std::memcpy(mat.data, data.data(), data.size() * sizeof(uint8_t));
        return mat;
    }
    return tl::unexpected("Unsupported data type for cv::Mat conversion");
}

tl::expected<void, std::string> NN::Tensor::reshape(const std::vector<int64_t>& newShape) {
    // Ensure all dimensions are positive
    for (int64_t dim : newShape) {
        if (dim <= 0) {
            return tl::unexpected("All dimensions must be positive in reshape.");
        }
    }

    // Calculate old and new sizes
    int64_t oldSize = 1;
    for (int64_t d : m_shape) oldSize *= d;

    int64_t newSize = 1;
    for (int64_t d : newShape) newSize *= d;

    if (oldSize != newSize) {
        return tl::unexpected("Reshape failed: total number of elements does not match.");
    }

    // Set new shape
    m_shape = newShape;
    return {};
}

int64_t NN::Tensor::numElements() const
{
    return shapeNumElements(m_shape);
}

int64_t NN::shapeNumElements(const std::vector<int64_t>& shape)
{
    return accumulate(shape.begin(), shape.end(), 1, std::multiplies());
}

std::string NN::shapeToString(const std::vector<int64_t>& shape)
{
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < shape.size(); ++i) {
        oss << shape[i];
        if (i + 1 < shape.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}