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
    // downsample images before computation
    cv::cuda::GpuMat gpu_from(from), gpu_to(to), gpu_flow(from.size(),CV_32FC2), gpu_down_from, gpu_down_to;
    if (downSampleFactor >= 1.0) {
        double reciprocalFactor = 1.0 / downSampleFactor;
        cv::cuda::resize(gpu_from, gpu_down_from, cv::Size(), reciprocalFactor, reciprocalFactor);
        cv::cuda::resize(gpu_to, gpu_down_to, cv::Size(), reciprocalFactor, reciprocalFactor);
    } else {
        gpu_down_from = gpu_from;
        gpu_down_to = gpu_to;
    }
    gpu_from.release();
    gpu_to.release();
    gpu_flow.release();
    // apply farneback
    m_farn->calc(gpu_down_from, gpu_down_to, gpu_flow);
    gpu_flow.download(flow);
    gpu_down_from.release();
    gpu_down_to.release();
    return true;
}
