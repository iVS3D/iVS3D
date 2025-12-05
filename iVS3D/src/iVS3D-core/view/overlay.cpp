#include "overlay.h"

#include <QBrush>
#include <QGraphicsTextItem>
#include <QImage>
#include <QPen>
#include <QPixmap>
#include <QRectF>
#include <opencv2/imgproc.hpp>

#include "textoverlayitem.h"

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const RectOverlay& overlay) {
    QPen pen(overlay.style.strokeColor);
    pen.setWidth(overlay.style.strokeWidth);
    pen.setCosmetic(true);  // pen width is independent of zoom level
    QBrush brush(overlay.style.fillColor);

    QGraphicsRectItem* item = new QGraphicsRectItem(overlay.rectangle, parent);
    item->setPen(pen);
    item->setBrush(brush);

    if (!parent) scene->addItem(item);
}

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const TextOverlay& overlay) {

    auto* item =
        new TextOverlayItem(overlay.text, overlay.style.font, overlay.style.textColor,
                            overlay.style.backgroundColor, overlay.anchor,
                            overlay.style.padding, parent);

    // Position item so that its anchor (0,0 in item coords)
    // lands at anchorScenePos
    item->setPos(overlay.position);

    // If you use a parent item that's already in the scene,
    // you do *not* need to add the item to the scene.
    if (!parent) {
        scene->addItem(item);
    }
}

QImage qImageFromCvMat(const cv::Mat& input, bool bgr) {
    cv::Mat rgb = input;
    if (input.channels() == 4) {
        if (bgr) {
            cv::cvtColor(input, rgb, cv::COLOR_BGRA2RGBA);
        }

        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGBA8888)
            .copy();
    } else if (rgb.channels() == 3) {
        if (bgr) {
            cv::cvtColor(input, rgb, cv::COLOR_BGR2RGB);
        }

        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGB888)
            .copy();
    } else if (rgb.channels() == 1) {
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_Grayscale8)
            .copy();
    }

    return QImage();
}

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const ImageOverlay& overlay) {
    // Create pixmap from cv::Mat
    QImage img = qImageFromCvMat(overlay.image, false);
    QPixmap pixmap = QPixmap::fromImage(img);

    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(pixmap, parent);
    item->setOpacity(overlay.style.opacity);
    item->setTransform(
        QTransform::fromScale(1.0/pixmap.width(), 1.0/pixmap.height()));
    if (!parent) {
        scene->addItem(item);
    }
}