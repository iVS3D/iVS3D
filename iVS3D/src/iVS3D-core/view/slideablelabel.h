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

    /**
     * @brief setStepSize sets the new step size
     * @param stepSize new step size
     */
    void setStepSize(float stepSize);

    /**
     * @brief getStepSize gets the step size
     * @return the step size
     */
    float getStepSize();

    /**
     * @brief enableSteps en/disables moving in steps
     * @param enable @a true if moving in steps, @a false if not
     */
    void enableSteps(bool enable);

signals:
    /**
     * @brief slided signals that the label has moved
     */
    void slided();

private:
    QPoint m_xIntervall = QPoint(0, 0);
    float m_stepSize = 0;
    bool m_stepsEnabled = false;
};

#endif // SLIDEABLELABEL_H
