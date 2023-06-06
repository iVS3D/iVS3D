#include "imagegatherercuda.h"

ImageGathererCuda::ImageGathererCuda(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames)
    : ImageGatherer(reader, downSampleFactor, futureFrames)
{

}

cv::Mat ImageGathererCuda::gatherSingleImage(uint frameIdx)
{
    cv::Mat readMat;
    readMat = m_reader->getPic(frameIdx);

    QElapsedTimer timer;
    timer.start();
    cv::cuda::GpuMat gpu_downMat, gpu_greyMat, gpu_readMat(readMat);
    cv::cuda::resize(gpu_readMat, gpu_downMat, cv::Size(), m_reciprocalDownSampleFactor, m_reciprocalDownSampleFactor);
    cv::cuda::cvtColor(gpu_downMat, gpu_greyMat, cv::COLOR_BGR2GRAY);
    cv::Mat outMat;
    gpu_greyMat.download(outMat);
    gpu_downMat.release();
    gpu_greyMat.release();
    gpu_greyMat.release();
    cv::cuda::resize(gpu_readMat, gpu_downMat, cv::Size(), m_reciprocalDownSampleFactor, m_reciprocalDownSampleFactor);
    qDebug() << "resize time: " << timer.elapsed() << "ms";
    return outMat;
}
