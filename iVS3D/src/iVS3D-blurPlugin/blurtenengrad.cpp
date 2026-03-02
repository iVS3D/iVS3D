#include "blurtenengrad.h"

#include <opencv2/imgproc.hpp>

#if defined(WITH_CUDA)
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#endif

extern bool g_useCuda;

BlurTenengrad::BlurTenengrad() {}
QString BlurTenengrad::getName() { return QStringLiteral("Tenengrad"); }

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

static double tenengradCPU(const cv::Mat &image, double edgeThreshold,
                           cv::Mat* debugImage = nullptr) {
    if (image.empty()) return -1.0;
    cv::Mat gray8;
    toGray8CPU(image, gray8);

    cv::Mat Gx, Gy;
    cv::Sobel(gray8, Gx, CV_32F, 1, 0, 3);
    cv::Sobel(gray8, Gy, CV_32F, 0, 1, 3);

    cv::Mat mag2 = Gx.mul(Gx) + Gy.mul(Gy);

    const float t2 = static_cast<float>(edgeThreshold * edgeThreshold);
    cv::Mat mask = (mag2 > t2);
    
    int strongCount = cv::countNonZero(mask);
    if (strongCount == 0) return 0.0;

    if (debugImage) {
        cv::Mat mask_f;
        mask.convertTo(mask_f, CV_32F, 1.0 / 255.0, 0.0);
        cv::multiply(mag2, mask_f, *debugImage);
    }

    return cv::mean(mag2, mask)[0];
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

static double tenengradCUDA(const cv::Mat &image, double edgeThreshold,
                            cv::Mat* debugImage = nullptr) {
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
        d_mag2, d_mask, d_maskF, d_masked;

    d_gray8.upload(gray8, tlsStream());
    d_gray8.convertTo(d_gray32f, CV_32F, 1.0, 0.0, tlsStream());

    sobelX->apply(d_gray32f, d_gx, tlsStream());
    sobelY->apply(d_gray32f, d_gy, tlsStream());

    cv::cuda::multiply(d_gx, d_gx, d_gx2, 1.0, -1, tlsStream());
    cv::cuda::multiply(d_gy, d_gy, d_gy2, 1.0, -1, tlsStream());
    cv::cuda::add(d_gx2, d_gy2, d_mag2, cv::noArray(), -1, tlsStream());

    const double t2 = edgeThreshold * edgeThreshold;
    cv::cuda::compare(d_mag2, t2, d_mask, cv::CMP_GT, tlsStream());  // 0/255
    
    d_mask.convertTo(d_maskF, CV_32F, 1.0 / 255.0, 0.0, tlsStream());
    cv::cuda::multiply(d_mag2, d_maskF, d_masked, 1.0, -1, tlsStream());

    if (debugImage) {
        d_masked.download(*debugImage, tlsStream());
        tlsStream().waitForCompletion();
    }

    const double sumStrong = reduceSum64(d_masked);
    const double count = reduceSum64(d_maskF);
    if (count <= 0.0) return 0.0;
    return sumStrong / count;
}
#endif

double BlurTenengrad::singleCalculation(const cv::Mat &image,
                                        cv::Mat* debugImage) {
#if defined(WITH_CUDA)
    if (g_useCuda) {
        return tenengradCUDA(image, m_edgeThreshold, debugImage);
    }
#endif
    return tenengradCPU(image, m_edgeThreshold, debugImage);
}
