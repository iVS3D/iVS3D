#include "flowcalculatorcuda.h"

FlowCalculatorCuda::FlowCalculatorCuda(int numLevels, double pyrScale, bool fastPyramids, int winSize, int numIters, int polyN, double polySigma, int flags)
{
    m_farn = cv::cuda::FarnebackOpticalFlow::create(numLevels, pyrScale, fastPyramids, winSize, numIters, polyN, polySigma, flags);
}

FlowCalculatorCuda::~FlowCalculatorCuda()
{
    m_farn.release();
    delete m_farn;
    m_farn = nullptr;
}

double FlowCalculatorCuda::calculateFlow(cv::Mat fromMat, cv::Mat toMat)
{
    if (!m_farn)
        return -1.0;

    cv::Mat flow;
    cv::cuda::GpuMat  gpu_from(fromMat), gpu_to(toMat), gpu_flow(fromMat.size(),CV_32FC2);
    m_farn->calc(gpu_from, gpu_to, gpu_flow);
    gpu_flow.download(flow);

    return flowMatToDouble(flow);
}
