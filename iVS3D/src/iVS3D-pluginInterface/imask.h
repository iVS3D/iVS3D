#pragma once

#include <QObject>
#include "ialgorithm.h"
#include "opencv2/core.hpp"
#include "ierror.h"
#include "resolution.h"
#include "roi.h"
#include <tl/expected.hpp>

using MaskResult = tl::expected<cv::Mat, Error>;

struct MaskData {
    uint index;     // Index of the image in the sequence
    cv::Mat image;  // The image to create a mask for
    Resolution workingResolution; // The resolution to work at
    ROI roi;        // The region of interest within the image
};

class IMask {
public:
    virtual ~IMask() = default;

    virtual MaskResult generateMask(const MaskData& data) = 0;
};

Q_DECLARE_INTERFACE(IMask, "iVS3D.IMask")