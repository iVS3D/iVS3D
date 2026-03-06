#pragma once

#include <QObject>

#include "opencv2/core.hpp"
#include "visualization.h"

namespace PLUG {

/**
 * @struct PreviewData
 * @brief Struct containing data required to generate a preview.
 * The image is resized to the working resolution and cropped to the region of
 * interest before being passed to the plugin.
 */
struct PreviewData {
    uint index;     // Index of the image in the sequence
    cv::Mat image;  // The image to create a preview for
};

/**
 * @interface IPreview
 * @ingroup Plugin
 * @brief The IPreview interface defines the contract for preview plugins in
 * iVS3D. Plugins implementing this interface are responsible for generating
 * visualizations for given preview data, which typically includes an image and
 * its index in a sequence. The generated visualization can include various
 * overlays and styles to enhance the preview experience.
 *
 * Plugins must implement the generatePreview method to create a Visualization
 * object based on the provided PreviewData. They may use the updatePreview
 * signal inherited from IBase to notify the system when a new preview should be
 * generated.
 *
 * @see @ref plugin_interface_doc "PluginInterface.md"
 *
 *
 * @author Dominik Wüst
 * @date 2025/12/05
 */
class IPreview {
   public:
    virtual ~IPreview() = default;

    /**
     * @brief Generates a preview visualization based on the provided data. This
     * function is executed asynchronously by iVS3D, such that expensive
     * operations such as neural network inference do not block the main thread
     * and GUI. Plugins should ensure that communication with the settings widget
     * is thread-safe, e.g. by using signals and slots!
     * 
     * @param data The PreviewData containing the image and its index.
     * @return A VisualizationResult containing either the generated
     * Visualization or an Error if the preview generation failed.
     */
    virtual VIS::VisualizationResult generatePreview(const PreviewData& data) = 0;
};

} // namespace PLUG

Q_DECLARE_INTERFACE(PLUG::IPreview, "iVS3D.IPreview")