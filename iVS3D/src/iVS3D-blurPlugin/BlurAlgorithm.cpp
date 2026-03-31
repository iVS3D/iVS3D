#include "BlurAlgorithm.h"

// Global CUDA switch controlled by Blur::sampleImages(useCuda)
bool g_useCuda = false;

#if defined(WITH_CUDA)
#include <opencv2/core/cuda.hpp>
#endif

double BlurAlgorithm::calcOneBluriness(const cv::Mat& img, cv::Mat* debugImage) {

#if defined(WITH_CUDA)
    if (g_useCuda) {
        static thread_local bool t_cudaInited = false;
        if (!t_cudaInited) {
            // Validate device count to avoid silent failures
            if (cv::cuda::getCudaEnabledDeviceCount() <= 0) {
                throw std::runtime_error("No CUDA devices available");
            }
            cv::cuda::setDevice(0);
            t_cudaInited = true;
        }
    }
#endif

    return singleCalculation(img, debugImage);
}
