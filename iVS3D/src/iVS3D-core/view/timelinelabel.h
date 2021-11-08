#ifndef TIMELINELABEL_H
#define TIMELINELABEL_H

#include <QLabel>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

/**
 * @class TimelineLabel
 *
 * @ingroup View
 *
 * @brief The TimelineLabel class is the background of the timeline, it displays positions of frames and keyframes
 *
 * @author Dominic Zahn
 *
 * @date 2021/04/12
 */
class TimelineLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * @brief TimelineLabel empty constructor
     * @param parent parent
     */
    explicit TimelineLabel(QWidget *parent = 0);

    /**
     * @brief adjustTimeline sets atributtes and redraws TimelineLabel
     * @param keyframes new keayframes
     * @param firstIndex new first index
     * @param lastIndex new last index
     * @param drawTimestamps should timestamps be drawn
     */
    void adjustTimeline(std::vector<uint> *keyframes, uint firstIndex, uint lastIndex, bool drawTimestamps);
    /**
     * @brief getFrameSize gets size of a frame as displayed in timeline
     * @return frame size
     */
    float getFrameSize();

    /**
     * @brief getFirstIndex gets the index of the first diplayed frame
     * @return first index
     */
    uint getFirstIndex();

    /**
     * @brief getLastIndexgets the index of the last diplayed frame
     * @return last index
     */
    uint getLastIndex();

    /**
     * @brief redraw redraws entire label
     */
    void redraw();

    /**
     * @brief mousePressEvent signals a mousepress event
     * @param ev the event
     */
    void mousePressEvent(QMouseEvent *ev);

signals:
    void sig_clicked(QPoint pos);

private:
    std::vector<uint> *m_keyframes = nullptr;
    uint m_firstIndex = 0;
    uint m_lastIndex = 0;
    bool m_drawTimestamps = true;

    QPixmap drawPixmap(bool timeStamps);
};

#endif // TIMELINELABEL_H
