#ifndef ROISELECT_H
#define ROISELECT_H

#include <QGraphicsScene>

/**
 * @class roiSelect
 *
 * @ingroup View
 *
 * @brief The roiSelect class provides the visual interface for the user to select a ROI and signals this interaction to the backend
 *
 * @author Daniel Brommer
 *
 * @date 2021/04/12
 */
class roiSelect : public QGraphicsScene
{  
     Q_OBJECT
public:
    /**
    * @brief roiSelect empty constructor
    * @param parent parent object
    */
   explicit roiSelect(QObject *parent = 0);

protected:
    /**
     * @brief mousePressEvent sends the mousepress signal when mouse is pressed on this scene
     * @param mouseEvent the event
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    /**
     * @brief mousePressEvent sends the mousemove signal when mouse is moved on this scene
     * @param mouseEvent the event
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);


signals:
    void sig_mousePress(QGraphicsSceneMouseEvent *mouseEvent);
    void sig_mouseMove(QGraphicsSceneMouseEvent *mouseEvent);
    void sig_mouseRelease(QGraphicsSceneMouseEvent *mouseEvent);
};

#endif // ROISELECT_H
