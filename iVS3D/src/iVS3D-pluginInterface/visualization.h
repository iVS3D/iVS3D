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
 * @defgroup Visualization Visualization
 * @brief Data model and style primitives for preview rendering in iVS3D.
 *
 * This group contains all overlay/view/result types in namespace VIS.
 */

/**
 * @namespace VIS
 * @brief Visualization data model namespace for plugin preview rendering.
 * @ingroup Visualization
 *
 * See @ref plugin_interface_doc "PluginInterface.md" for integration details.
 */
namespace VIS {

/**
 * @struct RectStyle
 * @brief Style settings for rendering rectangles.
 * @ingroup Visualization
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
 * @ingroup Visualization
 */
struct RectOverlay {
    QRectF rectangle;  // Rectangle coordinates in [0,1] normalized space
    RectStyle style;   // Style for rendering the rectangle
};

/**
 * @struct TextStyle
 * @brief Style settings for rendering text. All settings are in screen space
 * pixels and remain constant regardless of image size.
 * @ingroup Visualization
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
 * @ingroup Visualization
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
 * @ingroup Visualization
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
 * @ingroup Visualization
 */
struct ImageStyle {
    float opacity = 1.0f;  // Default opacity: fully opaque
    QRectF position = QRectF(0, 0, 1, 1);  // Default position: covers the entire image
};

/**
 * @struct ImageOverlay
 * @brief Represents an image overlay to be rendered on an image. The given
 * image will be drawn over the base image with the specified style. If
 * required, the image is resized to match the base image dimensions during
 * rendering.
 * @ingroup Visualization
 */
struct ImageOverlay {
    cv::Mat image;     // Image to overlay
    ImageStyle style;  // Style for rendering the image
};

/**
 * @typedef OverlayItem
 * @brief Variant type for all supported overlay primitives.
 * @ingroup Visualization
 */
using OverlayItem = std::variant<RectOverlay, TextOverlay, ImageOverlay>;

/**
 * @enum ViewportType
 * @brief Types of viewports for displaying images. This is only effective if
 * the region of interest is enabled in the video player. In that case,
 * FullImage shows the entire image, while RegionOfInterest only shows the area
 * defined by the region of interest.
 * @ingroup Visualization
 */
enum class ViewportType {
    FullImage,        // show the entire image
    RegionOfInterest  // show only the region of interest
};

/**
 * @struct ViewStyle
 * @brief Style settings for a view.
 *
 * The view can be mapped either to the full image or to the currently active
 * ROI viewport via `viewport`. The `relativeSize` value scales the view
 * against the selected viewport size (e.g. `(0.5, 0.5)` renders the view at
 * half width and half height of the mapped viewport).
 * @ingroup Visualization
 */
struct ViewStyle {
    QColor backgroundColor =
        Qt::transparent;  // transparent shows the original image
    ViewportType viewport =
        ViewportType::FullImage;  // whether to show only the region of interest
                                  // or the full image
    QPointF relativeSize = QPointF(
        1.0, 1.0);  // scale factor relative to viewport size (width, height)
    bool showTitle = true;        // if true, show the title of the view
};

/**
 * @struct View
 * @brief Represents one render target inside a plugin-generated visualization.
 *
 * A `View` is the basic building block emitted by `PLUG::IPreview`
 * implementations. A visualization may contain one or many views.
 *
 * Coordinate model:
 * - Overlays in `overlays` use a local normalized coordinate system in
 *   $[0,1]$ within this view.
 * - The host maps these local coordinates to either the full image or the ROI
 *   according to `style.viewport`.
 *
 * View mapping and size:
 * - `style.viewport == ViewportType::FullImage`: local space maps to the
 *   complete source image.
 * - `style.viewport == ViewportType::RegionOfInterest`: local space maps to
 *   the ROI only.
 * - `style.relativeSize` scales the view relative to its mapped viewport.
 *
 * Content:
 * - `overlays` contains one or more `OverlayItem`s (e.g. text, images,
 *   rectangles).
 * @ingroup Visualization
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
struct View {
    QString title;                      // Title of the view
    ViewStyle style;                    // Style for the view
    std::vector<OverlayItem> overlays;  // List of overlay items to render
};

/**
 * @struct Visualization
 * @brief Preview visualization container returned by `PLUG::IPreview` plugins.
 *
 * A `Visualization` is generated by plugins implementing
 * `PLUG::IPreview::generatePreview()` to display information in iVS3D.
 * It consists of one or more `View` entries in `views`.
 *
 * Each contained view:
 * - has its own local normalized coordinate space $[0,1]$,
 * - can target either full image or ROI mapping,
 * - can be scaled independently,
 * - can contain multiple overlay items (text, image overlays, boxes, ...).
 *
 * iVS3D manages final placement/layout of all views on screen.
 * @ingroup Visualization
 * @see @ref plugin_interface_doc "PluginInterface.md"
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
struct Visualization {
    std::vector<View> views;  // Views to visualize
};

/** 
 * @typedef VisualizationResult
 * @brief Type alias for the result of a visualization operation, which can be
 * either a successful Visualization or an Error indicating failure.
 * @ingroup Visualization
 */
using VisualizationResult = tl::expected<Visualization, PLUG::Error>;

} // namespace VIS
