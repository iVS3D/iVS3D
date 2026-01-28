#pragma once

#include <QObject>
#include <tl/expected.hpp>
#include <vector>

#include "ierror.h"
#include "resolution.h"
#include "roi.h"
#include "reader.h"
#include "opencv2/core.hpp"



struct SelectionData {
    std::vector<uint> selectedIndices; // Indices of the selected images
    Resolution workingResolution;      // Working resolution of the images
    ROI roi;                           // Region of interest within the images
    Reader* reader;                    // Pointer to the Reader instance
};

using SelectionResult = tl::expected<std::vector<uint>, Error>;

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

Q_DECLARE_INTERFACE(ISelection, "iVS3D.ISelection")