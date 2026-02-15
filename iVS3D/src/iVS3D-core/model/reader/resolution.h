#pragma once

#include <QtGlobal>
#include <QString>
#include <QRectF>
#include <QPoint>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

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

    bool operator!=(const Resolution& other) const { return !(*this == other); }


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
