#pragma once

#include <QtGlobal>
#include <QString>
#include <QRectF>
#include <QPoint>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "ISerializable.h"

/**
 * @brief The Resolution class encapsulates an image resolution (width and height). It provides functionality for parsing from a string,
 * converting from and to different Qt and OpenCV formats and for resizing images to the specified resolution.
 *
 * @author Dominik Wüst
 * @date March 2025
 */
class Resolution{
private:
    uint m_width;
    uint m_height;

public:
    /**
     * @brief Default constructor initializing width and height to zero. This resolution is not valid, but can i.e. be initialized from a string!
     */
    Resolution() : m_width(0), m_height(0) {}

    /**
     * @brief Constructs a Resolution object with specified width and height.
     * @param width The width of the resolution.
     * @param height The height of the resolution.
     */
    Resolution(uint width, uint height) : m_width(width), m_height(height) {}

    /**
     * @brief Constructs a Resolution object from an OpenCV Size object.
     * @param size OpenCV Size containing width and height.
     */
    Resolution(const cv::Size &size) : m_width(size.width), m_height(size.height) {}

    /**
     * @brief Constructs a Resolution object from a QSize object.
     * @param size QSize containing width and height.
     */
    Resolution(const QSize &size) : m_width(size.width()), m_height(size.height()) {}

    /**
     * @brief Constructs a Resolution object from a QPoint object.
     * @param size QPoint where x represents width and y represents height.
     */
    Resolution(const QPoint &size) : m_width(size.x()), m_height(size.y()) {}

    /**
     * @brief Constructs a Resolution object from an OpenCV Mat object.
     * @param img OpenCV Mat where width and height are extracted from its dimensions.
     */
    Resolution(const cv::Mat &img) : m_width(img.cols), m_height(img.rows) {}

    uint getWidth() const { return m_width; }
    uint getHeight() const { return m_height; }

    /**
     * @brief Checks whether the resolution is valid (i.e., width and height are greater than zero).
     * @return True if valid, false otherwise.
     */
    bool isValid() const { return m_width>0 && m_height>0; }

    /**
     * @brief Equality operator to compare two Resolution objects.
     * @param other The Resolution object to compare against.
     * @return True if both resolutions have the same width and height, false otherwise.
     */
    bool operator==(const Resolution& other) const { return m_width == other.m_width && m_height == other.m_height; }


    cv::Size toCvSize() const { return cv::Size(m_width, m_height); }
    QSize toQSize() const { return QSize(m_width, m_height); }
    QPoint toQPoint() const { return QPoint(m_width, m_height); }

    /**
     * @brief Resizes an OpenCV Mat image to the resolution defined in this object.
     * @param src Input image.
     * @param dest Output image (resized).
     */
    void resize(const cv::Mat &src, cv::Mat &dest) const { cv::resize(src, dest, this->toCvSize(), 0, 0, cv::INTER_AREA); }

    /**
     * @brief Resizes an OpenCV Mat image in-place.
     * @param img Image to resize.
     */
    void resize(cv::Mat &img) const { resize(img,img); }

    QString toString() const { return QString("%1 x %2").arg(m_width).arg(m_height); }

    /**
     * @brief Parses resolution from a string format.
     * @param resolution The string containing the resolution.
     * @return True if parsing was successful, false otherwise.
     */
    bool fromString(const QString& resolution);
};

/**
 * @brief The ROI class manages a region of interest represented as a rectangle in the [0,1]x[0,1] square.
 * We refer to this as a normalized ROI. The ROI can be scaled to a given resolution (width x height), in this case
 * we refert o it as a crop region. the crop region is only valid with the resolution it was scaled to because
 * the resulting QRect or OpenCV rectangle is gicen as pixels in [0,width]x[0,height] scale.
 *
 * @author Dominik Wüst
 * @date March 2025
 */
class ROI{
private:
    QRectF m_roi;

public:
    /**
     * @brief Default constructor initializing ROI to the full image (normalized [0,1] scale).
     */
    ROI() : m_roi(0.0,0.0,1.0,1.0) {}

    /**
     * @brief Constructs an ROI object from a QRectF.
     * @param roi The QRectF defining the region in [0,1] scale.
     */
    ROI(const QRectF &roi) : m_roi(roi) {}

    /**
     * @brief Constructs an ROI object from an OpenCV Rect2f.
     * @param roi The OpenCV rectangle defining the region in [0,1] scale.
     */
    ROI(const cv::Rect2f &roi) : m_roi(roi.x,roi.y,roi.width,roi.height) {}

    /**
     * @brief Constructs an ROI object from a QRect in pixel coordinates and the corresponding reolution for normalization.
     * @param roi The QRect defining the region in [0,width]x[0,height] scale.
     * @param resolution The Resolution (width x height) for normalizing the roi to [0,1] scale.
     */
    ROI(const QRect &roi, const Resolution &resolution);
    /**
     * @brief Constructs an ROI object from an OpenCV Rect in pixel coordinates and the corresponding reolution for normalization.
     * @param roi The OpenCV rectangle defining the region in [0,width]x[0,height] scale.
     * @param resolution The Resolution (width x height) for normalizing the roi to [0,1] scale.
     */
    ROI(const cv::Rect &roi, const Resolution &resolution);

    /**
     * @brief Checks if the ROI matches the default ROI which is the entire image.
     * @return true if the ROI matches is the entire image.
     */
    bool isDefault() const { return m_roi == QRectF(0.0,0.0,1.0,1.0); }

    cv::Rect2f toCvRect() const { return cv::Rect2f(m_roi.left(), m_roi.top(), m_roi.width(), m_roi.height()); }
    QRectF toQRectF() const { return QRectF(m_roi.left(), m_roi.top(), m_roi.width(), m_roi.height()); }

    /**
     * @brief Converts to a QRect, scaled to the given resolution.
     * @param resolution The resolution (width x height) to scale the roi to.
     * @return The ROI as a QRect in [0,width]x[0,height] scale.
     */
    QRect cropAsQRect(const Resolution& resolution) const { return QRect(m_roi.left() * resolution.getWidth(), m_roi.top() * resolution.getHeight(), m_roi.width() * resolution.getWidth(), m_roi.height() * resolution.getHeight()); }

    /**
     * @brief Converts to an OpenCV rect, scaled to the given resolution.
     * @param resolution The resolution (width x height) to scale the roi to.
     * @return The ROI as an OpenCV rectangle in [0,width]x[0,height] scale.
     */
    cv::Rect cropAsCvRect(const Resolution& resolution) const { return cv::Rect(m_roi.left() * resolution.getWidth(), m_roi.top() * resolution.getHeight(), m_roi.width() * resolution.getWidth(), m_roi.height() * resolution.getHeight()); }

    /**
     * @brief Crops an image using the ROI and stores the result in dest.
     * @param src Input image.
     * @param dest Output cropped image.
     */
    void crop(const cv::Mat &src, cv::Mat &dest) const { dest = src(cropAsCvRect(Resolution(src))); }

    /**
     * @brief Crops an image in-place.
     * @param img Image to crop.
     */
    void crop(cv::Mat &img) const { crop(img,img); }
};

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

    Resolution getWorkingResolution() {return m_workingResolution;}
    Resolution getOriginalResolution() {return m_originalResolution;}
    ROI getRoi() {return m_roi;}
    bool getUseRoi() {return m_useRoi;}

    QVariant toText() override;
    void fromText(QVariant data) override;
};
