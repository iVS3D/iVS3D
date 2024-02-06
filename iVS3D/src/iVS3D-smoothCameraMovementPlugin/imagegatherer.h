#ifndef IMAGEGATHERER_H
#define IMAGEGATHERER_H

#include <QObject>
#include <QHash>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>
#include <mutex>
#include <opencv2/core.hpp>

#include "reader.h"

/**
 * @class ImageGatherer
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGatherer class is an interface which defines the structure for the
 *        hardware specific algorithm. The algorithm reads, resizes and greyscales an frame.
 *        This algorithm can be executed on multiple frames at once.
 *
 * @author Dominic Zahn
 *
 * @date 2022/04/12
 */
class ImageGatherer : public QObject
{
    Q_OBJECT
public:
    ImageGatherer(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames);
    QPair<cv::Mat, cv::Mat> gatherImagePair(uint from, uint to);

protected:
    /**
     * @brief gatherSingleImage reads, resizes and greyscales a single frame
     * @param frameIdx defines which frame should be gathered
     * @return image of requested frame
     */
    virtual cv::Mat gatherSingleImage(uint frameIdx) = 0;

    Reader *m_reader;
    double m_reciprocalDownSampleFactor = 0.0;
    QHash<uint, cv::Mat> m_bufferedImages;

private:
    bool checkStoredImages(uint idx, cv::Mat *out);
    static cv::Mat gatherSingleImageStatic(uint frameIdx, ImageGatherer *imgg);
};

#endif // IMAGEGATHERER_H
