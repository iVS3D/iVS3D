#include "imagegatherercpu.h"

ImageGathererCpu::ImageGathererCpu(Reader *reader, std::vector<uint> futureFrames)
    : ImageGatherer(reader, futureFrames)
{

}

cv::Mat ImageGathererCpu::gatherSingleImage(uint frameIdx)
{
    cv::Mat readMat, greyMat;
    readMat = m_reader->getPic(frameIdx);

    if (readMat.empty())
        return cv::Mat();

    cv::cvtColor(readMat, greyMat, cv::COLOR_BGR2GRAY);

    return greyMat;
}
