#pragma once
/**
 * @file overlay.h
 * @brief Functions to draw various overlay types onto a QGraphicsScene.
 *
 * These functions facilitate the rendering of different overlay elements such
 * as rectangles, text, and images onto a QGraphicsScene, enabling rich
 * visualization capabilities. They are separated from the data structures
 * defined in visualization.h as they focus solely on rendering and do not need
 * to be exposed to the plugins.
 *
 * @author Dominik Wüst
 * @date 2025/12/04
 */
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QRect>

#include "visualization.h"

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const VIS::RectOverlay& overlay);

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const VIS::TextOverlay& overlay);

void drawOverlay(QGraphicsScene* scene, QGraphicsItem* parent,
                 const VIS::ImageOverlay& overlay);