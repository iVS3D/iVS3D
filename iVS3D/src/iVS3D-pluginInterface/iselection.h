#pragma once

#include <QObject>
#include <tl/expected.hpp>
#include <vector>
#include <memory>

#include "ierror.h"
#include "resolution.h"
#include "roi.h"
#include "reader.h"
#include "LogFileParent.h"
#include "opencv2/core.hpp"

namespace PLUG {

/**
 * @struct SelectionData
 * @brief Data available for image selection in ISelection plugins.
 * 
 * @details This includes the indices of the selected images, the working resolution,
 * the region of interest, a pointer to the Reader instance for accessing image data,
 * and a shared pointer to a log file for recording selection details.
 * 
 * @ingroup Plugin
 * @see ISelection
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
struct SelectionData {
    std::vector<uint> selectedIndices; // Indices of the selected images
    Resolution workingResolution;      // Working resolution of the images
    ROI roi;                           // Region of interest within the images
    Reader* reader;                    // Pointer to the Reader instance
    std::shared_ptr<LogFileParent> logFile;  // Log sink for plugin selection
};

/**
 * @typedef SelectionResult
 * @brief Type alias for the result of an image selection operation, which can be
 * either a successful vector of selected image indices or an Error indicating failure.
 * @ingroup Plugin
 */
using SelectionResult = tl::expected<std::vector<uint>, Error>;

/**
 * @class ISelection
 * @brief Interface for keyframe/image-selection plugins in iVS3D.
 * @ingroup Plugin
 * @see @ref plugin_interface_doc "PluginInterface.md"
 */
class ISelection {
   public:
    virtual ~ISelection() = default;

    /**
     * @brief Selects images based on the provided selection data.
     *
     * This method should be implemented by the plugin to select images
     * according to the specified criteria in SelectionData.
     *
     * @param data The SelectionData containing selected indices, working resolution, and ROI.
     * @param cancelFlag A volatile boolean flag that can be set to true to cancel the selection process.
     * @return A SelectionResult containing either the selected image indices or an Error if the selection failed.
     */
    virtual SelectionResult selectImages(const SelectionData& data, volatile bool& cancelFlag) = 0;
};

} // namespace PLUG

Q_DECLARE_INTERFACE(PLUG::ISelection, "iVS3D.ISelection")