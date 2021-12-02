#ifndef TIMELINE_H
#define TIMELINE_H

#include <QWidget>
#include <QSpinBox>
#include <QShortcut>
#include <QKeySequence>
#include <QScreen>
#include "view/slideablelabel.h"
#include "view/timelinelabel.h"
#include "ui_timeline.h"
#include <cmath>


namespace Ui {
class Timeline;
}

/**
 * @class Timeline
 *
 * @ingroup View
 *
 * @brief The Timeline class is the class wich coordinates and manages all timeline events and elements
 *
 * @author Dominic Zahn
 *
 * @date 2021/04/12
 */
class Timeline : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Timeline creates all elements and connects them
     * @param parent parent
     */
    explicit Timeline(QWidget *parent = nullptr);
    ~Timeline();
    
    /**
     * @brief updateKeyframes redraws timeline with new keyframe positions
     * @param newKeyframes keyframe list
     */
    void updateKeyframes(const std::vector<uint> &newKeyframes);
    
    /**
     * @brief setFrames sets the frame
     * @param keyframes new keyframes
     * @param frameCount new keyframe count
     */
    void setFrames(const std::vector<uint> &keyframes, uint frameCount);
    
    /**
     * @brief selectedFrame returns the index of the frame which is currently selected
     * @return index
     */
    uint selectedFrame();
    
    /**
     * @brief selectFrame moves markers to new positions
     * @param index new frame index
     */
    void selectFrame(uint index);
    
    /**
     * @brief updateHighlighterWidth redraws highlighter with new width
     */
    void updateHighlighterWidth();
    
    /**
     * @brief resizeEvent redraws timeline when window resizes
     * @param ev the resize event
     */
    void resizeEvent(QResizeEvent *ev) override;
    
    /**
     * @brief setEnabled en/disables the timeline
     * @param enable @a true if disable, @a false if enable
     */
    void setEnabled(bool enable);
    
    /**
     * @brief getBoundaries gets the boundaries of the working set
     * @return lower and upper boundary
     */
    QPoint getBoundaries();

signals:
    void sig_selectedChanged(uint index);

private slots:
    void highlighterMoved();
    void markerMoved();
    void zoomChanged();
    void sbIndexChanged(int index);
    void startBoundaryMoved();
    void endBoundaryMoved();
    void slot_resetSc();
    void slot_totalTimelineClicked(QPoint pos);
    void slot_zoomTimelineClicked(QPoint pos);

public slots:
    /**
     * @brief resize redraws the timeline when window resizes
     */
    void resize();

private:
    Ui::Timeline *ui;
    // ui elements
    SlideableLabel *m_highlighter = 0; // used for highlighting the zoomed area
    SlideableLabel *m_marker = 0; // used to mark the currently shown frame
    TimelineLabel *m_zoomTimeline = 0; // displays a only highlighted frames
    TimelineLabel *m_totalTimeline = 0; // displays every frame
    SlideableLabel *m_startBoundaryLabel = 0; // marks the start boundary of the corpped frames
    SlideableLabel *m_endBoundaryLabel = 0; // marks the end boundary of the cropped frames
    QDoubleSpinBox *m_zoomSpinBox = 0; // adjust width of the highlighter (=> zoom)
    QSpinBox *m_indexSpinBox = 0; // shows and changes current index

    QPoint getHighlighterRange();
    void setupSpinBoxes();
    QPixmap drawMarker(uint pixWidth, uint pixHeight, uint symbolWidth, uint symbolHeigth, uint topMargin);

    // cropped boundaries
    QPoint m_boundaries;
    uint calcBoundary(SlideableLabel *boundaryLabel);
    QPixmap drawBoundary(uint pixWidth, uint pixHeight, uint symbolWidth, uint topBottomMargin, bool isStart);
    void positionBoundaries(uint startPos, uint endPos);

    // shortcuts
    QShortcut *m_resetSc;

    // frame data
    std::vector<uint> m_keyframes;
    uint m_frameCount = 1;

    bool m_selectActive = false;
    bool m_enableWindow = false;
};

#endif // TIMELINE_H
