#ifndef IMAGEGATHERERCUDA_H
#define IMAGEGATHERERCUDA_H

#include "imagegatherer.h"

#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>

#include <QElapsedTimer>
#include <QDebug>

/**
 * @class ImageGathererCuda
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGathererCuda class implements the ImageGatherer interface as a
 *        cuda specific algorithm. The algorithm reads, resizes and greyscales an frame.
 *        This algorithm can be executed on multiple frames at once.
 */
class ImageGathererCuda : public ImageGatherer
{
public:
    ImageGathererCuda(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames);
private:
    /**
     * @brief gatherSingleImage reads, resizes and greyscales a single frame
     * @param frameIdx defines which frame should be gathered
     * @return image of requested frame
     */
    cv::Mat gatherSingleImage(uint frameIdx) override;
};

#endif // IMAGEGATHERERCUDA_H
