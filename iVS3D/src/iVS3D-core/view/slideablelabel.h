#ifndef SLIDEABLELABEL_H
#define SLIDEABLELABEL_H

#include <QLabel>
#include <QWidget>
#include <QMouseEvent>

/**
 * @class SlideableLabel
 *
 * @ingroup View
 *
 * @brief The SlideableLabel class is the label which sits on the timeline and can be moved on it
 *
 * @author Dominic Zahn
 *
 * @date 2021/04/12
 */
class SlideableLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * @brief SlideableLabel constructs a new label
     * @param parent parent of this label
     */
    explicit SlideableLabel(QWidget *parent = 0);

    /**
     * @brief mouseMoveEvent sets the label to the position of the mouse movement
     * @param ev the event
     */
    void mouseMoveEvent(QMouseEvent *ev);

    /**
     * @brief setIntervall sets the new intervall
     * @param xRange new intervall
     */
    void setIntervall(QPoint xRange);

    /**
     * @brief getIntervall gets the intervall
     * @return intervall
     */
    QPoint getIntervall();

    /**
     * @brief setYLevel sets the new yLevel
     * @param y new yLevel
     */
    void setYLevel(int y);

    /**
     * @brief setHeight sets the new height
     * @param height new height
     */
    void setHeight(uint height);

    /**
     * @brief setWidth sets the new width
     * @param width new height
     */
    void setWidth(uint width);

    /**
     * @brief setRelPosition sets the new relative position
     * @param delta new relative position?
     */
    void setRelPosition(float delta);

    /**
     * @brief getRelPosition gets the relative position
     * @return the relative position
     */
    uint getRelPosition();

signals:
    /**
     * @brief mouseMoved signals that the mouseMoveEvent was triggered on the label and sends the x coordinate
     * @param xMovement represents the x coordinate of the mouseMoveEvent
     */
    void mouseMoved(int xMovement);

private:
    QPoint m_xIntervall = QPoint(0, 0);
};

#endif // SLIDEABLELABEL_H
