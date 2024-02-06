#ifndef IMAGEGATHERERCPU_H
#define IMAGEGATHERERCPU_H

#include "imagegatherer.h"

#include <opencv2/video/tracking.hpp>

/**
 * @class ImageGathererCpu
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGathererCpu class implements the ImageGatherer interface as a
 *        cuda specific algorithm. The algorithm reads, resizes and greyscales an frame.
 *        This algorithm can be executed on multiple frames at once.
 */
class ImageGathererCpu : public ImageGatherer
{
public:
    ImageGathererCpu(Reader *reader, double downSampleFactor, std::vector<uint> futureFrames);
private:
    /**
     * @brief gatherSingleImage reads, resizes and greyscales a single frame
     * @param frameIdx defines which frame should be gathered
     * @return image of requested frame
     */
    cv::Mat gatherSingleImage(uint frameIdx) override;
};

#endif // IMAGEGATHERERCPU_H
