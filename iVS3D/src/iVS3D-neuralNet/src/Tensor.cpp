#include "Tensor.h"

namespace NN
{

    tl::expected<Tensor, std::string> Tensor::fromCvMat(const cv::Mat &mat)
    {
        if (mat.empty())
        {
            return tl::unexpected("cv::Mat is empty");
        }

        int channels = mat.channels();
        int height = mat.rows;
        int width = mat.cols;

        Shape shape = {channels, height, width};

        if (mat.depth() == CV_8U)
        {
            std::vector<uint8_t> chw_data;
            chw_data.reserve(channels * height * width);

            std::vector<cv::Mat> splitted;
            cv::split(mat, splitted);

            for (int c = 0; c < channels; ++c)
            {
                chw_data.insert(chw_data.end(), splitted[c].datastart, splitted[c].dataend);
            }

            return Tensor::fromData(std::move(chw_data), shape);
        }

        if (mat.depth() == CV_32F)
        {
            std::vector<float> chw_data;
            chw_data.reserve(channels * height * width);

            std::vector<cv::Mat> splitted;
            cv::split(mat, splitted);

            for (int c = 0; c < channels; ++c)
            {
                const float *channelData = splitted[c].ptr<float>();
                chw_data.insert(chw_data.end(), channelData, channelData + height * width);
            }

            return Tensor::fromData(std::move(chw_data), shape);
        }

        return tl::unexpected("Unsupported cv::Mat type (only CV_8U and CV_32F are supported)");
    }

    tl::expected<Tensor, std::string> Tensor::fromCvMat(const cv::Mat &mat, const Shape &shape, float scale, std::vector<float> mean, std::vector<float> std)
    {
        if (mat.empty())
        {
            return tl::unexpected("Input image is empty");
        }

        if (shape.size() < 2)
        {
            return tl::unexpected("Invalid shape, expected at least [H,W] or more dimensions");
        }

        if (mean.size() > 0 && mean.size() != mat.channels())
        {
            return tl::unexpected("Mean vector size does not match number of channels in cv::Mat");
        }

        if (std.size() > 0 && std.size() != mat.channels())
        {
            return tl::unexpected("Std vector size does not match number of channels in cv::Mat");
        }

        if (std::any_of(std.begin(), std.end(), [](float s)
                        { return s == 0.0f; }))
        {
            return tl::unexpected("Std vector must not contain zero values");
        }

        // grayscale images (shape = [H,W])
        if (mat.channels() == 1)
        {

            // check if all shape dimensions are 1 or -1 except the last two
            for (auto dim = shape.begin(); dim != shape.end() - 2; ++dim)
            {
                if (*dim != 1 && *dim != -1)
                {
                    return tl::unexpected("Invalid shape for grayscale image, expected [H,W] or [...,H,W] with leading dimensions being 1 or -1");
                }
            }

            // check if mean vector is valid
            if (mean.size() > 1)
            {
                return tl::unexpected("Mean vector for grayscale image must not have more than one element");
            }

            cv::Mat tmp;

            // convert to float and resize if necessary
            cv::Size size(shape[shape.size() - 2], shape[shape.size() - 1]);
            if (size != mat.size())
            {
                cv::resize(mat, tmp, size, 0, 0, cv::INTER_AREA);
                tmp.convertTo(tmp, CV_32F, scale);
            }
            else
            {
                mat.convertTo(tmp, CV_32F, scale);
            }

            // subtract mean if provided
            if (mean.size() == 1)
            {
                tmp -= mean[0];
            }

            // divide by std if provided
            if (std.size() == 1)
            {
                tmp /= std[0];
            }
            // create the tensor
            return fromCvMat(tmp);
        }

        // validate the channel dimension matches the cv::Mat channels or is dynamic (if present)
        if ((shape.size() > 2) && (shape[shape.size() - 3] != mat.channels()) && (shape[shape.size() - 3] != -1))
        {
            return tl::unexpected("Shape does not match number of channels in cv::Mat. Expected " + std::to_string(mat.channels()) + " channels, got " + std::to_string(shape[shape.size() - 2]));
        }

        // check if all shape dimensions are 1 or -1 except the last three
        for (size_t i = 0; i < shape.size() - 3; ++i)
        {
            if (shape[i] != 1 && shape[i] != -1)
            {
                return tl::unexpected("Invalid shape, expected [N,C,H,W] or [...,C,H,W] with leading dimensions being 1 or -1");
            }
        }

        if (!mean.empty() && std.empty())
        {
            std.resize(mean.size(), 1.0f); // If mean is provided, std must also be provided
        }

        cv::Mat tmp = mean.empty() ? preprocessCvMat(mat, shape, scale)
                                   : preprocessCvMat(mat, shape, scale, mean, std);

        // create the tensor
        return fromCvMat(tmp);
    }

    tl::expected<Tensor, std::string> Tensor::fromCvMats(const std::vector<cv::Mat> &mats)
    {
        if (mats.empty())
        {
            return tl::unexpected("Input vector of cv::Mat is empty");
        }

        if (mats[0].depth() == CV_8U)
        {
            return fromCvMatsTyped<uint8_t, CV_8U>(mats);
        }
        else if (mats[0].depth() == CV_32F)
        {
            return fromCvMatsTyped<float, CV_32F>(mats);
        }

        return tl::unexpected("Unsupported cv::Mat depth");
    }

    tl::expected<Tensor, std::string> Tensor::fromCvMats(const std::vector<cv::Mat> &mats, const Shape &shape, float scale, std::vector<float> mean, std::vector<float> std)
    {
        if (mats.empty())
        {
            return tl::unexpected("Input vector of cv::Mat is empty");
        }

        if (shape.size() != 4)
        {
            return tl::unexpected("Invalid shape, expected [N,C,H,W] layout!");
        }

        if (mean.size() != 0 && mean.size() != mats[0].channels())
        {
            return tl::unexpected("Mean vector size does not match number of channels in cv::Mat");
        }

        if (std.size() != 0 && std.size() != mats[0].channels())
        {
            return tl::unexpected("Std vector size does not match number of channels in cv::Mat");
        }

        if (std::any_of(std.begin(), std.end(), [](float s)
                        { return s == 0.0f; }))
        {
            return tl::unexpected("Std vector must not contain zero values");
        }

        int channels = mats[0].channels();
        int height = mats[0].rows;
        int width = mats[0].cols;

        if (!mean.empty() && std.empty())
        {
            std.resize(mean.size(), 1.0f); // If mean is provided, std must also be provided
        }

        // preprocess all cv::Mats individually
        std::vector<cv::Mat> preprocessedMats;
        preprocessedMats.reserve(mats.size());
        for (const auto &mat : mats)
        {
            if (mat.empty())
            {
                return tl::unexpected("One of the input cv::Mat is empty");
            }
            cv::Mat preprocessed = mean.empty() ? preprocessCvMat(mat, shape, scale)
                                                : preprocessCvMat(mat, shape, scale, mean, std);

            preprocessedMats.push_back(preprocessed);
        }

        return fromCvMats(preprocessedMats);
    }

    std::string Tensor::toString() const
    {

        std::ostringstream oss;
        oss << "Tensor(shape=[";
        for (size_t i = 0; i < m_shape.size(); ++i)
        {
            oss << m_shape[i];
            if (i + 1 < m_shape.size())
                oss << ", ";
        }
        oss << "], dtype=";
        oss << NN::toString(dtype());
        oss << ")";
        return oss.str();
    }

    tl::expected<cv::Mat, std::string> Tensor::toCvMat() const
    {
        if (m_shape.size() < 1 || m_shape.size() > 4)
        {
            return tl::unexpected("Unsupported tensor shape for toCvMat()");
        }

        int channels = 1;
        int height = 0, width = 0;

        if (m_shape.size() == 1)
        {
            // 1D tensor, convert to cv::Mat as row-vector
            height = 1;
            width = static_cast<int>(m_shape[0]);
        }
        else if (m_shape.size() == 2)
        {
            // HW
            height = static_cast<int>(m_shape[0]);
            width = static_cast<int>(m_shape[1]);
        }
        else if (m_shape.size() == 3)
        {
            // CHW
            channels = static_cast<int>(m_shape[0]);
            height = static_cast<int>(m_shape[1]);
            width = static_cast<int>(m_shape[2]);
        }
        else if (m_shape.size() == 4)
        {
            // NCHW with N == 1
            if (m_shape[0] != 1)
            {
                return tl::unexpected("Only NCHW with N=1 supported for toCvMat()");
            }
            channels = static_cast<int>(m_shape[1]);
            height = static_cast<int>(m_shape[2]);
            width = static_cast<int>(m_shape[3]);
        }

        // Handle float or uint8_t
        return std::visit([&](const auto &data) -> tl::expected<cv::Mat, std::string>
                          {
        using T = typename std::decay<decltype(data)>::type::value_type;

        int type = 0;
        if constexpr (std::is_same<T, float>::value) {
            type = CV_32FC(channels);
        } else if constexpr (std::is_same<T, uint8_t>::value) {
            type = CV_8UC(channels);
        } else {
            return tl::unexpected("Unsupported tensor data type");
        }

        if (static_cast<size_t>(channels * height * width) != data.size()) {
            return tl::unexpected("Tensor shape does not match data size");
        }

        // Convert CHW to HWC layout
        std::vector<T> hwcData(height * width * channels);
        for (int c = 0; c < channels; ++c) {
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    hwcData[h * width * channels + w * channels + c] =
                        data[c * height * width + h * width + w];
                }
            }
        }

        // Create cv::Mat with shared ownership of hwcData (copy for now)
        cv::Mat mat(height, width, type);
        std::memcpy(mat.data, hwcData.data(), hwcData.size() * sizeof(T));
        return mat; }, m_data);
    }

    tl::expected<void, std::string> Tensor::reshape(const Shape &newShape)
    {
        // Ensure all dimensions are positive
        for (int64_t dim : newShape)
        {
            if (dim <= 0)
            {
                return tl::unexpected("All dimensions must be positive in reshape.");
            }
        }

        // Calculate old and new sizes
        int64_t oldSize = 1;
        for (int64_t d : m_shape)
            oldSize *= d;

        int64_t newSize = 1;
        for (int64_t d : newShape)
            newSize *= d;

        if (oldSize != newSize)
        {
            return tl::unexpected("Reshape failed: total number of elements does not match.");
        }

        // Set new shape
        m_shape = newShape;
        return {};
    }

    int64_t Tensor::numElements() const
    {
        return shapeNumElements(m_shape);
    }

    int64_t shapeNumElements(const Shape &shape)
    {
        return accumulate(shape.begin(), shape.end(), 1, std::multiplies());
    }

    std::string shapeToString(const Shape &shape)
    {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < shape.size(); ++i)
        {
            oss << shape[i];
            if (i + 1 < shape.size())
                oss << ", ";
        }
        oss << "]";
        return oss.str();
    }

    int64_t shapeToStride(const Shape &shape, uint64_t dim)
    {
        uint64_t stride = 1;
        for (uint64_t i = dim + 1; i < shape.size(); ++i)
            stride *= shape[i];
        return stride;
    }

    // In-place versions
    tl::expected<void, std::string> Tensor::squeeze()
    {
        m_shape.erase(std::remove(m_shape.begin(), m_shape.end(), 1), m_shape.end());
        return {};
    }

    tl::expected<void, std::string> Tensor::squeeze(int64_t axis)
    {
        if (axis < 0 || axis >= static_cast<int64_t>(m_shape.size()))
            return tl::unexpected("Axis out of bounds in squeeze");

        if (m_shape[axis] != 1)
            return tl::unexpected("Cannot squeeze dimension that is not size 1");

        m_shape.erase(m_shape.begin() + axis);
        return {};
    }

    tl::expected<void, std::string> Tensor::unsqueeze(int64_t axis)
    {
        if (axis < 0 || axis > static_cast<int64_t>(m_shape.size()))
            return tl::unexpected("Axis out of bounds in unsqueeze");

        m_shape.insert(m_shape.begin() + axis, 1);
        return {};
    }
}