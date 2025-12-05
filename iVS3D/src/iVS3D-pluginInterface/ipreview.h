#pragma once
/**
 * @file ipreview.h
 * @brief Interface for preview plugins in iVS3D.
 * 
 * @ingroup Plugin
 * @author Dominik Wüst
 * @date 2025/12/05
 */

#include <QObject>
#include "visualization.h"
#include "opencv2/core.hpp"

struct PreviewData {
    uint index;     // Index of the image in the sequence
    cv::Mat image;  // The image to create a preview for
};

/**
 * @interface IPreview
 * @ingroup Plugin
 * @brief The IPreview interface defines the contract for preview plugins in iVS3D. Plugins implementing this interface
 * are responsible for generating visualizations for given preview data, which typically includes an image and its index
 * in a sequence. The generated visualization can include various overlays and styles to enhance the preview experience.
 * 
 * Plugins must implement the generatePreview method to create a Visualization object based on the provided PreviewData.
 * They may use the updatePreview signal inherited from IBase to notify the system when a new preview should be generated.
 * 
 * 
 * @author Dominik Wüst
 * @date 2025/12/05
 */
class IPreview
{
public:
    virtual ~IPreview() = default;

    virtual Visualization generatePreview(const PreviewData& data) = 0;

};

Q_DECLARE_INTERFACE(IPreview, "iVS3D.IPreview")