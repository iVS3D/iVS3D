#ifndef TIMELINELABEL_H
#define TIMELINELABEL_H

#include <QLabel>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

// color definitions
#define INBOUND_COLOR Qt::red
#define OUTOFBOUND_COLOR Qt::darkGray
#define TIMESTAMP_COLOR Qt::black

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
     * @param indexBounds represents the bounds of the timelinelabel in form of indices.
     *        They can be represented as floads so that the first and last displayed frame has an offset to its boundarie.
     * @param (optional) boundaries defines which keyframelines will be outside of the bound and will color them diffrent
     * @param drawTimestamps should timestamps be drawn
     */
    void updateTimelinelabel(std::vector<uint> *keyframes, QPointF indexBounds, bool drawTimestamps, QPoint boundaries = QPoint(-1,-1));

    /**
     * @brief relPosToIndex returns the respective index as float number to the relative position the timelinelabel
     * @param relativePosition on the timelinelinelabel in pixels
     * @return index as a float number to also represent a position between two frames
     */
    float relPosToIndex(int relativePosition);

    /**
     * @brief indexToRelPos returns the relative position which correspondes to the global index (firstIndex computation is involved in function)
     * @param index as a float number to also represent a position between two frames
     * @return relative position on the timelinelinelabel in pixels
     */
    uint indexToRelPos(float index);

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
     *
     * @param (optional) boundaries defines which frames are defined as out of bound and should be colored diffrent
     */
    void redraw(QPoint boundaries = QPoint(-1, -1));

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
    float m_firstIndex_offset = 0.f;
    float m_lastIndex_offset = 0.f;
    float m_frameSize = 0.f;
    bool m_drawTimestamps = true;

    QPixmap drawPixmap(bool timeStamps, QPoint boundaries);
};

#endif // TIMELINELABEL_H
