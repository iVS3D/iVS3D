#include "blursobel.h"

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

static double sobelCPU(const cv::Mat &image, cv::Mat* debugImage = nullptr) {
    if (image.empty()) return -1.0;
    cv::Mat gray8;
    toGray8CPU(image, gray8);

    cv::Mat Gx, Gy;
    cv::Sobel(gray8, Gx, CV_64F, 1, 0, 3);
    cv::Sobel(gray8, Gy, CV_64F, 0, 1, 3);
    cv::Mat FM = Gx.mul(Gx) + Gy.mul(Gy);
    if (debugImage) {
        *debugImage = FM;
    }
    return cv::mean(FM).val[0];
}

#if defined(WITH_CUDA)
static inline cv::cuda::Stream &tlsStream() {
    thread_local cv::cuda::Stream s;
    return s;
}

static double reduceSum64(const cv::cuda::GpuMat &src) {
    cv::cuda::GpuMat tmp, tmp2;
    cv::cuda::reduce(src, tmp, 0, cv::REDUCE_SUM, CV_64F, tlsStream());
    cv::cuda::reduce(tmp, tmp2, 1, cv::REDUCE_SUM, CV_64F, tlsStream());
    cv::Mat h;
    tmp2.download(h, tlsStream());
    tlsStream().waitForCompletion();
    return h.at<double>(0, 0);
}

static double sobelCUDA(const cv::Mat &image, cv::Mat* debugImage = nullptr) {
    if (image.empty()) return -1.0;

    cv::Mat gray8;
    toGray8CPU(image, gray8);

    thread_local cv::Ptr<cv::cuda::Filter> sobelX;
    thread_local cv::Ptr<cv::cuda::Filter> sobelY;
    if (sobelX.empty())
        sobelX = cv::cuda::createSobelFilter(CV_32F, CV_32F, 1, 0, 3);
    if (sobelY.empty())
        sobelY = cv::cuda::createSobelFilter(CV_32F, CV_32F, 0, 1, 3);

    thread_local cv::cuda::GpuMat d_gray8, d_gray32f, d_gx, d_gy, d_gx2, d_gy2,
        d_sum;

    d_gray8.upload(gray8, tlsStream());
    d_gray8.convertTo(d_gray32f, CV_32F, 1.0, 0.0, tlsStream());

    sobelX->apply(d_gray32f, d_gx, tlsStream());
    sobelY->apply(d_gray32f, d_gy, tlsStream());

    cv::cuda::multiply(d_gx, d_gx, d_gx2, 1.0, -1, tlsStream());
    cv::cuda::multiply(d_gy, d_gy, d_gy2, 1.0, -1, tlsStream());
    cv::cuda::add(d_gx2, d_gy2, d_sum, cv::noArray(), -1, tlsStream());

    const double N =
        static_cast<double>(d_sum.rows) * static_cast<double>(d_sum.cols);
    if (N <= 0.0) return 0.0;

    if (debugImage) {
        d_sum.download(*debugImage, tlsStream());
        tlsStream().waitForCompletion();
    }

    const double total = reduceSum64(d_sum);
    return total / N;
}
#endif

BlurSobel::BlurSobel() {}

QString BlurSobel::getName() { return m_name; }

double BlurSobel::singleCalculation(const cv::Mat &image, cv::Mat* debugImage) {
#if defined(BLUR_PLUGIN_OPENCV_HAS_CUDA_LINK)
    if (g_useCuda) {
        return sobelCUDA(image, debugImage);
    }
#endif
    return sobelCPU(image, debugImage);
}
