#include "farnebackoptflowcpu.h"

FarnebackOptFlowCPU::FarnebackOptFlowCPU()
{
    qDebug() << "Farneback for CPU created";
}

FarnebackOptFlowCPU::~FarnebackOptFlowCPU()
{
    m_farn.release();
    delete m_farn;
}

bool FarnebackOptFlowCPU::setup(int numLevels, double pyrScale, bool fastPyramids, int winSize, int numIters, int polyN, double polySigma, int flags)
{
    m_farn = cv::FarnebackOpticalFlow::create(numLevels,pyrScale,fastPyramids,winSize,numIters,polyN,polySigma,flags);
    m_isSetup = true;
    return m_isSetup;
}

bool FarnebackOptFlowCPU::calculateFlow(const cv::Mat &from, const cv::Mat &to, cv::Mat &flow, double downSampleFactor)
{
    if(!m_isSetup){
        return false;
    }

    if (downSampleFactor >= 1.f) {
        double reciprocalFactor = 1.0 / downSampleFactor;
        cv::Mat down_from, down_to;
        cv::resize(from, down_from, cv::Size(), reciprocalFactor, reciprocalFactor);
        cv::resize(to, down_to, cv::Size(), reciprocalFactor, reciprocalFactor);
        m_farn->calc(down_from, down_to, flow);
    } else {
        m_farn->calc(from, to, flow);
    }
    return true;
}
