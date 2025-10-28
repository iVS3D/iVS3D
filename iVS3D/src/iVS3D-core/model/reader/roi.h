#pragma once

#include <QtGlobal>
#include <QString>
#include <QRectF>
#include <QPoint>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "resolution.h"

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
