#include "roiselect.h"

roiSelect::roiSelect(QObject *parent) : QGraphicsScene(parent)
{

}



void roiSelect::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    emit sig_mousePress(mouseEvent);
}

void roiSelect::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    emit sig_mouseMove(mouseEvent);
}

