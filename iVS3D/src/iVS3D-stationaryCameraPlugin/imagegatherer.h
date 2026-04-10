#ifndef IMAGEGATHERER_H
#define IMAGEGATHERER_H

#include <QHash>
#include <QObject>
#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <numeric>
#include <opencv2/core.hpp>
#include <vector>

#include "reader.h"

/**
 * @class ImageGatherer
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGatherer class is an interface which defines the structure
 * for the hardware specific algorithm. The algorithm reads, resizes and
 * greyscales an frame. This algorithm can be executed on multiple frames at
 * once.
 *
 * @author Dominic Zahn
 *
 * @date 2022/04/12
 */
class ImageGatherer : public QObject {
    Q_OBJECT
   public:
    ImageGatherer(iReader* reader, std::vector<uint> futureFrames);
    QPair<cv::Mat, cv::Mat> gatherImagePair(uint from, uint to);

   protected:
    /**
     * @brief gatherSingleImage reads, resizes and greyscales a single frame
     * @param frameIdx defines which frame should be gathered
     * @return image of requested frame
     */
    virtual cv::Mat gatherSingleImage(uint frameIdx) = 0;

    iReader* m_reader;
    QHash<uint, cv::Mat> m_bufferedImages;

   private:
    bool checkStoredImages(uint idx, cv::Mat* out);
    static cv::Mat gatherSingleImageStatic(uint frameIdx, ImageGatherer* imgg);
};

#endif  // IMAGEGATHERER_H
