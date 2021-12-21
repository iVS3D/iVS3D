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

bool FarnebackOptFlowGPU::calculateFlow(const cv::Mat &from, const cv::Mat &to, cv::Mat &flow)
{
    cv::cuda::GpuMat gpu_prev(from), gpu_curr(to), gpu_flow(from.size(),CV_32FC2);
    m_farn->calc(gpu_prev, gpu_curr, gpu_flow);
    gpu_flow.download(flow);
    gpu_prev.release();
    gpu_curr.release();
    gpu_flow.release();
    return true;
}
