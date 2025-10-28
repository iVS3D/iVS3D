#include "imagegatherercuda.h"

ImageGathererCuda::ImageGathererCuda(Reader *reader, std::vector<uint> futureFrames)
    : ImageGatherer(reader, futureFrames)
{

}

cv::Mat ImageGathererCuda::gatherSingleImage(uint frameIdx)
{
    cv::Mat readMat = m_reader->getPic(frameIdx);

    if (readMat.empty())
        return cv::Mat();

    cv::cuda::GpuMat gpu_greyMat, gpu_readMat(readMat);
    cv::cuda::cvtColor(gpu_readMat, gpu_greyMat, cv::COLOR_BGR2GRAY);
    cv::Mat outMat;
    gpu_greyMat.download(outMat);
    gpu_greyMat.release();
    return outMat;
}
