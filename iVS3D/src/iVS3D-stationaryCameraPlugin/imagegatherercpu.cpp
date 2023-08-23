#include "imagegatherercpu.h"

ImageGathererCpu::ImageGathererCpu(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames)
    : ImageGatherer(reader, downSampleFactor, futureFrames)
{

}

cv::Mat ImageGathererCpu::gatherSingleImage(uint frameIdx)
{
    cv::Mat readMat, downMat, greyMat;
    readMat = m_reader->getPic(frameIdx);

    if (readMat.empty())
        return cv::Mat();

    cv::resize(readMat, downMat, cv::Size(), m_reciprocalDownSampleFactor, m_reciprocalDownSampleFactor);
    cv::cvtColor(downMat, greyMat, cv::COLOR_BGR2GRAY);

    return greyMat;
}
