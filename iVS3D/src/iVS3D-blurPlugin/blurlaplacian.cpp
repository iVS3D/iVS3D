#include "blurlaplacian.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#if defined(WITH_CUDA)
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#endif

extern bool g_useCuda;

static inline void toGray8CPU(const cv::Mat &src, cv::Mat &gray8) {
    cv::Mat gray;
    if (src.channels() == 1) {
        gray = src;
    } else {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }

    if (gray.depth() != CV_8U) {
        double minv, maxv;
        cv::minMaxLoc(gray, &minv, &maxv);
        double alpha = (maxv > minv) ? (255.0 / (maxv - minv)) : 1.0;
        double beta = (maxv > minv) ? (-minv * alpha) : 0.0;
        gray.convertTo(gray8, CV_8U, alpha, beta);
    } else {
        gray8 = gray;
    }
}

static double laplacianCPU(const cv::Mat &image, cv::Mat* debugImage = nullptr) {
    if (image.empty()) return -1.0;
    cv::Mat gray8;
    toGray8CPU(image, gray8);
    cv::Mat lap;
    cv::Laplacian(gray8, lap, CV_64F, 1);
    cv::Scalar mean, stddev;
    cv::meanStdDev(lap, mean, stddev);
    if (debugImage) {
        lap = cv::abs(lap);
        *debugImage = lap;
    }
    return stddev[0] * stddev[0];
}

#if defined(WITH_CUDA)
static inline cv::cuda::Stream &tlsStream() {
    thread_local cv::cuda::Stream s;
    return s;
}

// Reduce to a scalar double using the provided stream
static double reduceSum64(const cv::cuda::GpuMat &src) {
    cv::cuda::GpuMat tmp, tmp2, out;
    cv::cuda::reduce(src, tmp, 0, cv::REDUCE_SUM, CV_64F,
                     tlsStream());  // rows -> 1xW
    cv::cuda::reduce(tmp, tmp2, 1, cv::REDUCE_SUM, CV_64F,
                     tlsStream());  // 1xW -> 1x1
    cv::Mat h;
    tmp2.download(h, tlsStream());
    tlsStream().waitForCompletion();
    return h.at<double>(0, 0);
}

static inline void ensureCudaThreadInit() {
    static thread_local bool t_inited = false;
    if (!t_inited) {
        if (cv::cuda::getCudaEnabledDeviceCount() <= 0) {
            throw std::runtime_error("No CUDA devices available");
        }
        cv::cuda::setDevice(0);
        t_inited = true;
    }
}

static double laplacianCUDA(const cv::Mat &image, cv::Mat* debugImage) {
    if (image.empty()) return -1.0;

    cv::Mat gray8;
    toGray8CPU(image, gray8);

    thread_local cv::Ptr<cv::cuda::Filter> lap;
    if (lap.empty()) {
        // srcType == dstType is required by cudafilters
        lap = cv::cuda::createLaplacianFilter(CV_32F, CV_32F, 1);
    }

    thread_local cv::cuda::GpuMat d_gray8, d_gray32f, d_lap32f, d_sq32f;

    d_gray8.upload(gray8, tlsStream());
    d_gray8.convertTo(d_gray32f, CV_32F, 1.0, 0.0, tlsStream());
    lap->apply(d_gray32f, d_lap32f, tlsStream());
    cv::cuda::multiply(d_lap32f, d_lap32f, d_sq32f, 1.0, -1, tlsStream());

    const double N =
        static_cast<double>(d_lap32f.rows) * static_cast<double>(d_lap32f.cols);
    if (N <= 0.0) return 0.0;

    const double sum = reduceSum64(d_lap32f);
    const double sumsq = reduceSum64(d_sq32f);

    const double mean = sum / N;
    const double ex2 = sumsq / N;
    const double var = ex2 - mean * mean;

    if (debugImage) {
        cv::Mat h_lap32f;
        cv::cuda::abs(d_lap32f, d_lap32f, tlsStream());
        d_lap32f.download(h_lap32f, tlsStream());
        tlsStream().waitForCompletion();
        *debugImage = h_lap32f;
    }
    return var;
}
#endif

BlurLaplacian::BlurLaplacian() {}
QString BlurLaplacian::getName() { return m_name; }

double BlurLaplacian::singleCalculation(const cv::Mat &image, cv::Mat* debugImage) {
#if defined(WITH_CUDA)
    if (g_useCuda) {
        return laplacianCUDA(image, debugImage);
    }
#endif
    return laplacianCPU(image, debugImage);
}
