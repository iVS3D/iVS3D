#include "farnebackoptflowgpu.h"

FarnebackOptFlowGPU::FarnebackOptFlowGPU()
{
    qDebug() << "Farneback on GPU created";
}

FarnebackOptFlowGPU::~FarnebackOptFlowGPU()
{
    // clear gpu storage
    m_farn.release();
    delete m_farn;
}

bool FarnebackOptFlowGPU::setup(int numLevels, double pyrScale, bool fastPyramids, int winSize, int numIters, int polyN, double polySigma, int flags)
{
    m_farn = cv::cuda::FarnebackOpticalFlow::create(numLevels,pyrScale,fastPyramids,winSize,numIters,polyN,polySigma,flags);
    m_isSetup = true;
    return m_isSetup;
}

bool FarnebackOptFlowGPU::calculateFlow(const cv::Mat &from, const cv::Mat &to, cv::Mat &flow, double downSampleFactor)
{
//    // downsample images before computation
//    cv::cuda::GpuMat gpu_from(from), gpu_to(to), gpu_flow(from.size(),CV_32FC2), gpu_down_from, gpu_down_to;
//    if (downSampleFactor >= 1.0) {
//        double reciprocalFactor = 1.0 / downSampleFactor;
//        cv::cuda::resize(gpu_from, gpu_down_from, cv::Size(), reciprocalFactor, reciprocalFactor);
//        cv::cuda::resize(gpu_to, gpu_down_to, cv::Size(), reciprocalFactor, reciprocalFactor);
//    } else {
//        gpu_down_from = gpu_from;
//        gpu_down_to = gpu_to;
//    }
//    gpu_from.release();
//    gpu_to.release();
//    // apply farneback
//    cv::cuda::GpuMat gpu_grey_from, gpu_grey_to;
//    cv::cuda::cvtColor(gpu_down_from, gpu_grey_from, cv::COLOR_BGR2GRAY);
//    cv::cuda::cvtColor(gpu_down_to, gpu_grey_to, cv::COLOR_BGR2GRAY);
//    gpu_down_from.release();
//    gpu_down_to.release();
//    m_farn->calc(gpu_grey_from, gpu_grey_to, gpu_flow);
//    gpu_grey_from.release();
//    gpu_grey_to.release();

    if (!m_isSetup) {
        return false;
    }

    cv::cuda::GpuMat  gpu_from(from), gpu_to(to), gpu_flow(from.size(),CV_32FC2);
    m_farn->calc(gpu_from, gpu_to, gpu_flow);
    gpu_flow.download(flow);

    return true;
}
