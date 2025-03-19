#pragma once

#include <QtGlobal>
#include <QString>
#include <QRectF>
#include <QPoint>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "ISerializable.h"
#include "resolution.h"
#include "roi.h"


/**
 * @brief The ReaderParams class manages the working resolution (WRes) and region of interest (ROI) for reader-objects. It provides access to the original image resolution
 * as well as the current WRes and enforces constraints for a valid WRes, i.e. WRes is not larger than the original resolution in any dimension.
 *
 * @author Dominik Wüst
 * @date March 2025
 */
class ReaderParams : ISerializable {
private:
    Resolution m_originalResolution;
    Resolution m_workingResolution;
    ROI m_roi;
    bool m_useRoi;

public:
    /**
     * @brief Default constructor initializing resolutions to zero and ROI usage to false.
     */
    ReaderParams() : m_originalResolution(), m_workingResolution(), m_useRoi(false){}

    /**
     * @brief Initializes the ReaderParams with an original resolution. The working resolution is also set to this resolution by default.
     * @param originalResolution The resolution of the original image.
     */
    void initialize(const Resolution &originalResolution);

    /**
     * @brief Sets the working resolution if it is valid (not exceeding original resolution).
     * @param resolution The new working resolution.
     * @return True if the resolution was updated successfully, false otherwise.
     */
    bool setWorkingResolution(const Resolution& resolution);

    /**
     * @brief Sets the region of interest (ROI).
     * @param roi The new ROI.
     * @return True if successfully set.
     */
    bool setRoi(const ROI& roi);

    /**
     * @brief Enables or disables ROI usage.
     * @param useRoi Whether to apply ROI cropping.
     * @return Always returns true.
     */
    bool setUseRoi(bool useRoi) { m_useRoi=useRoi; return true;}

    Resolution getWorkingResolution() const {return m_workingResolution;}
    Resolution getOriginalResolution() const {return m_originalResolution;}
    ROI getRoi() const {return m_roi;}
    bool getUseRoi() const {return m_useRoi;}

    QVariant toText() override;
    void fromText(QVariant data) override;
};
