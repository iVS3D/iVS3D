#include "imagegatherer.h"

ImageGatherer::ImageGatherer(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames)
{
    m_reciprocalDownSampleFactor = 1.0 / downSampleFactor;
    m_reader = reader;
    m_reader->initMultipleAccess(futureFrames);
}

QPair<cv::Mat, cv::Mat> ImageGatherer::gatherImagePair(uint from, uint to)
{
    cv::Mat fromMat, toMat;
    std::future<cv::Mat> handleFrom, handleTo;
    // gather images from reader if they are not buffered
    if (!checkStoredImages(from, &fromMat)) {
        handleFrom = std::async(std::launch::async, gatherSingleImageStatic, from, this);
    }
    if (!checkStoredImages(to, &toMat)) {
        handleTo = std::async(std::launch::async, gatherSingleImageStatic, to, this);
    }

    // retrieve gathered images and store them in buffer
    if (handleFrom.valid()) {
        handleFrom.wait();
        fromMat = handleFrom.get();
        m_bufferedImages.insert(from, fromMat);
    }
    if (handleTo.valid()) {
        handleTo.wait();
        toMat = handleTo.get();
        m_bufferedImages.insert(to, toMat);
    }

    return QPair<cv::Mat, cv::Mat>(fromMat, toMat);
}

bool ImageGatherer::checkStoredImages(uint idx, cv::Mat *out)
{
    // check if image was already computed
    if (m_bufferedImages.contains(idx)) {
        *out = m_bufferedImages.value(idx);
        // remove entry because each frame wil only be used two times
        m_bufferedImages.remove(idx);
        return true;
    } else {
        return false;
    }
}

cv::Mat ImageGatherer::gatherSingleImageStatic(uint frameIdx, ImageGatherer *imgg)
{
    return imgg->gatherSingleImage(frameIdx);
}
