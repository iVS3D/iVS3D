#include "flowcalculatorcpu.h"

FlowCalculatorCpu::FlowCalculatorCpu(int numLevels, double pyrScale, bool fastPyramids, int winSize, int numIters, int polyN, double polySigma, int flags)
{
    m_farn = cv::FarnebackOpticalFlow::create(numLevels, pyrScale, fastPyramids, winSize, numIters, polyN, polySigma, flags);
}

FlowCalculatorCpu::~FlowCalculatorCpu()
{
    m_farn.release();
    delete m_farn;
    m_farn = nullptr;
}

double FlowCalculatorCpu::calculateFlow(cv::Mat fromMat, cv::Mat toMat)
{
    if (!m_farn)
        return -1.0;
    if (fromMat.empty() || toMat.empty())
        return -1.0;

    cv::Mat flow(fromMat.size(),CV_32FC2);
    try {
        m_farn->calc(fromMat, toMat, flow);
    }  catch (cv::Exception cvEx) {
        qDebug() << "Exception in farnback calculation: " << QString::fromStdString(cvEx.msg);
    }
    return flowMatToDouble(flow);
}
