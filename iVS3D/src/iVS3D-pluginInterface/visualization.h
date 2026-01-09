#pragma once
#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <tl/expected.hpp>
#include <variant>

#include "ierror.h"
#include "opencv2/core.hpp"

/**
 * @struct RectStyle
 * @brief Style settings for rendering rectangles.
 */
struct RectStyle {
    QColor strokeColor = Qt::green;  // Default color: Green
    int strokeWidth = 2;             // Default thickness in screen space pixels
    QColor fillColor = Qt::transparent;  // Default fill color: Transparent
};

/**
 * @struct RectOverlay
 * @brief Represents a rectangle overlay to be rendered on an image. The
 * rectangle is defined in the normalized [0,1] plane and will be projected to
 * the image dimensions during rendering. The strokeWidth is defined in screen
 * space pixels and remains constant regardless of image size.
 */
struct RectOverlay {
    QRectF rectangle;  // Rectangle coordinates in [0,1] normalized space
    RectStyle style;   // Style for rendering the rectangle
};

/**
 * @struct TextStyle
 * @brief Style settings for rendering text. All settings are in screen space
 * pixels and remain constant regardless of image size.
 */
struct TextStyle {
    QColor textColor = Qt::white;        // Default text color: White
    int fontSize = 12;                   // Default font size in points
    QFont font = QFont();                // Default font: System default
    qreal padding = 3.0;                 // Default padding in pixels
    QColor backgroundColor = Qt::black;  // Default background color: Black
};

/**
 * @enum TextAnchor
 * @brief Anchor positions for text overlays.
 */
enum class TextAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/**
 * @struct TextOverlay
 * @brief Represents a text overlay to be rendered on an image. The position is
 * defined in the normalized [0,1] plane and will be projected to the image
 * dimensions during rendering. The anchor defines which part of the text aligns
 * to the specified position.
 */
struct TextOverlay {
    QString text;      // Text content to display
    QPointF position;  // Position in [0,1] normalized space
    TextAnchor anchor = TextAnchor::BottomLeft;  // Default anchor position
    TextStyle style;                             // Style for rendering the text
};

/**
 * @struct ImageStyle
 * @brief Style settings for rendering images.
 */
struct ImageStyle {
    float opacity = 1.0f;  // Default opacity: fully opaque
};

/**
 * @struct ImageOverlay
 * @brief Represents an image overlay to be rendered on an image. The given
 * image will be drawn over the base image with the specified style. If
 * required, the image is resized to match the base image dimensions during
 * rendering.
 */
struct ImageOverlay {
    cv::Mat image;     // Image to overlay
    ImageStyle style;  // Style for rendering the image
};

using OverlayItem = std::variant<RectOverlay, TextOverlay, ImageOverlay>;

/**
 * @enum ViewportType
 * @brief Types of viewports for displaying images. This is only effective if
 * the region of interest is enabled in the video player. In that case,
 * FullImage shows the entire image, while RegionOfInterest only shows the area
 * defined by the region of interest.
 */
enum class ViewportType {
    FullImage,        // show the entire image
    RegionOfInterest  // show only the region of interest
};

/**
 * @struct ViewStyle
 * @brief Style settings for a view.
 */
struct ViewStyle {
    QColor backgroundColor =
        Qt::transparent;  // transparent shows the original image
    ViewportType viewport =
        ViewportType::FullImage;  // whether to show only the region of interest
                                  // or the full image
    bool showTitle = true;        // if true, show the title of the view
};

/**
 * @struct View
 * @brief Represents a single view in the visualization, containing a title,
 * style settings, and a list of overlay items to render. The view can display
 * multiple overlays such as rectangles, text, and images on top of the base
 * image or the selected region of interest, depending on the ViewportType.
 */
struct View {
    QString title;                      // Title of the view
    ViewStyle style;                    // Style for the view
    std::vector<OverlayItem> overlays;  // List of overlay items to render
};

/**
 * @struct Visualization
 * @brief Represents the complete visualization consisting of multiple views.
 * Each view can contain various overlays and has its own style settings. iVS3D
 * will manage the layout of these views when displaying the visualization.
 */
struct Visualization {
    std::vector<View> views;  // Views to visualize
};

/** 
 * @typedef VisualizationResult
 * @brief Type alias for the result of a visualization operation, which can be
 * either a successful Visualization or an Error indicating failure.
 */
using VisualizationResult = tl::expected<Visualization, Error>;