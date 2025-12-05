#pragma once
#include "opencv2/core.hpp"

#include <QColor>
#include <QRectF>
#include <QPointF>
#include <QFont>
#include <variant>

struct RectStyle {
    QColor strokeColor = Qt::green;     // Default color: Green
    int strokeWidth = 2;                        // Default thickness in screen space pixels
    QColor fillColor = Qt::transparent;   // Default fill color: Semi-transparent red
};

struct RectOverlay {
    QRectF rectangle;           // Rectangle coordinates in [0,1] normalized space
    RectStyle style;           // Style for rendering the rectangle
};

struct TextStyle {
    QColor textColor = Qt::white;       // Default text color: White
    int fontSize = 12;                  // Default font size in points
    QFont font = QFont();               // Default font: System default
    qreal padding = 3.0;                // Default padding in pixels
    QColor backgroundColor = Qt::black; // Default background color: Black
};

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

struct TextOverlay {
    QString text;               // Text content to display
    QPointF position;          // Position in [0,1] normalized space
    TextAnchor anchor = TextAnchor::BottomLeft; // Default anchor position
    TextStyle style;           // Style for rendering the text
};

struct ImageStyle {
    float opacity = 1.0f;               // Default opacity: fully opaque
};

struct ImageOverlay {
    cv::Mat image;             // Image to overlay
    ImageStyle style;          // Style for rendering the image
};

using OverlayItem = std::variant<RectOverlay, TextOverlay, ImageOverlay>;

enum class ViewportType {
    FullImage,          // show the entire image
    RegionOfInterest    // show only the region of interest
};

struct ViewStyle {
    QColor backgroundColor = Qt::transparent;   // transparent shows the original image
    ViewportType viewport = ViewportType::FullImage; // whether to show only the region of interest or the full image
    bool showTitle = true;                      // if true, show the title of the view
};

struct View {
    QString title;                      // Title of the view
    ViewStyle style;                    // Style for the view
    std::vector<OverlayItem> overlays;  // List of overlay items to render
};

struct Visualization {
    std::vector<View> views;  // Views to visualize
};