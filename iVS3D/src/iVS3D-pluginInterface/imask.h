#pragma once

#include <QObject>
#include <tl/expected.hpp>

#include "ierror.h"
#include "opencv2/core.hpp"
#include "resolution.h"
#include "roi.h"

/**
 * @typedef MaskResult
 * @brief Type alias for the result of a mask generation operation, which can be
 * either a successful cv::Mat mask or an Error indicating failure.
 */
using MaskResult = tl::expected<cv::Mat, Error>;

/**
 * @struct MaskData
 * @brief Struct containing data required to generate a binary mask.
 * The image is resized to the working resolution and cropped to the region of
 * interest before being passed to the plugin.
 */
struct MaskData {
    uint index;     // Index of the image in the sequence
    cv::Mat image;  // The image to create a mask for
};

/**
 * @interface IMask
 * @brief Interface for mask generation plugins in iVS3D.
 *
 * Plugins implementing this interface can generate binary masks for images
 * based on provided settings. The generated masks are exported alongside the
 * sampled images for use in 3D reconstruction.
 *
 * @date 2025/12/28
 * @author Dominik Wüst
 */
class IMask {
   public:
    virtual ~IMask() = default;
    /**
     * @brief Generates a binary mask for the given image data.
     *
     * This method should be implemented by the plugin to create a binary mask
     * based on the provided MaskData. The image in MaskData is already resized
     * to the working resolution and cropped to the region of interest.
     *
     * @param data The MaskData containing the image and its index.
     * @return A MaskResult containing either the generated cv::Mat mask or an
     * Error if the mask generation failed.
     */
    virtual MaskResult generateMask(const MaskData& data) = 0;
};

Q_DECLARE_INTERFACE(IMask, "iVS3D.IMask")