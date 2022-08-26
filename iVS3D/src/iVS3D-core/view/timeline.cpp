#include "timeline.h"

Timeline::Timeline(QWidget *parent) :
    QWidget(parent),
   ui(new Ui::Timeline)
{
    ui->setupUi(this);

    // create highlighter
    m_highlighter = new SlideableLabel(this);
    m_highlighter->setStyleSheet("background-color: rgba(0, 139, 208, 150 );");
    m_highlighter->setFrameStyle(QFrame::Box);
    // create marker
    m_marker = new SlideableLabel(this);
    m_marker->setWidth(10);
    m_marker->setFrameStyle(QFrame::NoFrame);
    QPixmap pix = drawMarker(m_marker->width() - m_marker->lineWidth(), m_marker->height() - m_marker->lineWidth() - 1, 6, m_marker->height() * 0.5, m_marker->height() * 0.1);
    m_marker->setPixmap(pix);

    const uint boundariesWidth = QGuiApplication::primaryScreen()->size().width() / 20;
    // create start boundary-marker
    m_startBoundaryLabel = new SlideableLabel(this);
    m_startBoundaryLabel->setWidth(boundariesWidth);
    m_startBoundaryLabel->setFrameShape(QFrame::NoFrame);
    m_startBoundaryLabel->setPixmap(drawBoundary(m_startBoundaryLabel->width(), m_startBoundaryLabel->height(), m_startBoundaryLabel->height() / 5, m_startBoundaryLabel->height() / 5, true));
    // create end boundary-marker
    m_endBoundaryLabel = new SlideableLabel(this);
    m_endBoundaryLabel->setWidth(boundariesWidth);
    m_endBoundaryLabel->setFrameShape(QFrame::NoFrame);
    m_endBoundaryLabel->setPixmap(drawBoundary(m_endBoundaryLabel->width(), m_endBoundaryLabel->height(), m_startBoundaryLabel->height() / 5, m_startBoundaryLabel->height() / 5, false));

    m_totalTimeline = ui->label_totalTimeline;
    m_zoomTimeline = ui->label_zoomTimeline;
    m_zoomSpinBox = ui->spinBox_zoom;
    m_indexSpinBox = ui->spinBox_index;

    // connect slideEvents to the Timeline
    QObject::connect(this->m_highlighter, &SlideableLabel::mouseMoved, this, &Timeline::highlighterMoved);
    QObject::connect(this->m_marker, &SlideableLabel::mouseMoved, this, &Timeline::markerMoved);

    // connect spinBoxes
    QObject::connect(m_zoomSpinBox, SIGNAL(valueChanged(double)), this, SLOT(zoomChanged()));
    QObject::connect(m_indexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Timeline::sbIndexChanged);

    // connect slideEvents of the moving boundarie labels
    QObject::connect(m_startBoundaryLabel, &SlideableLabel::mouseMoved, this, [=](int x){ boundaryMoved(x, m_startBoundaryLabel); });
    QObject::connect(m_endBoundaryLabel, &SlideableLabel::mouseMoved, this, [=](int x){ boundaryMoved(x, m_endBoundaryLabel); });

    // connect clickEvents of the timelineLabels
    QObject::connect(m_zoomTimeline, &TimelineLabel::sig_clicked, this, &Timeline::slot_zoomTimelineClicked);
    QObject::connect(m_totalTimeline, &TimelineLabel::sig_clicked, this, &Timeline::slot_totalTimelineClicked);

    // disable window
    setEnabled(false);
}

Timeline::~Timeline()
{
    delete ui;
}

void Timeline::updateKeyframes(const std::vector<uint> &newKeyframes)
{
    this->m_keyframes = newKeyframes;
    resize();
}

void Timeline::resize()
{
    int currentImage = m_indexSpinBox->value();
    // setup highlighter
    m_highlighter->setYLevel(m_totalTimeline->parentWidget()->y());
    m_highlighter->setHeight(m_totalTimeline->parentWidget()->height());
    updateHighlighterWidth();

    // set frames for each timelineLabel
    m_totalTimeline->updateTimelinelabel(&this->m_keyframes, QPointF(0, m_frameCount - 1), false, m_boundaries);
    m_zoomTimeline->updateTimelinelabel(&this->m_keyframes, getHighlighterRange(), true, m_boundaries);

    // setup marker
    m_marker->setYLevel(m_zoomTimeline->parentWidget()->y());
    m_marker->setHeight(m_zoomTimeline->parentWidget()->height());
    uint markerMinX = m_zoomTimeline->parentWidget()->x() - m_marker->width() / 2 - m_marker->lineWidth() + 3;
    uint markerMaxX = markerMinX + m_zoomTimeline->width();
    m_marker->setIntervall(QPoint(markerMinX, markerMaxX));
    // set bounderies for the second time to adjust the maxX
    markerMaxX = m_zoomTimeline->indexToRelPos(m_frameCount - 1);
    m_marker->setIntervall(QPoint(markerMinX, markerMaxX));

    // setup start boundary marker
    m_startBoundaryLabel->setYLevel(m_totalTimeline->parentWidget()->y());
    m_startBoundaryLabel->setHeight(m_totalTimeline->parentWidget()->height());
    uint startBoundaryMinX = m_totalTimeline->parentWidget()->x() - m_startBoundaryLabel->width() / 2 - m_startBoundaryLabel->lineWidth();
    uint startBoundaryMaxX = startBoundaryMinX + m_zoomTimeline->width();
    m_startBoundaryLabel->setIntervall(QPoint(startBoundaryMinX, startBoundaryMaxX));
    // setup end boudary marker
    m_endBoundaryLabel->setYLevel(m_totalTimeline->parentWidget()->y());
    m_endBoundaryLabel->setHeight(m_totalTimeline->parentWidget()->height());
    uint endBoundaryMinX = m_totalTimeline->parentWidget()->x() - m_startBoundaryLabel->width() / 2 - m_startBoundaryLabel->lineWidth();
    uint endBoundaryMaxX = endBoundaryMinX + m_zoomTimeline->width();
    m_endBoundaryLabel->setIntervall(QPoint(endBoundaryMinX, endBoundaryMaxX));

    // correct position after resizing
    positionBoundaries(m_boundaries.x(), m_boundaries.y());

    //show last image
    sbIndexChanged(currentImage);
}

void Timeline::setFrames(const std::vector<uint> &keyframes, uint frameCount)
{
    this->m_keyframes = keyframes;
    this->m_frameCount = frameCount;

    // reset boundaries to min and max postion
    positionBoundaries(0, m_frameCount - 1);

    resize();
    setupSpinBoxes();
}

uint Timeline::selectedFrame()
{
    uint selected = qRound(m_zoomTimeline->relPosToIndex(m_marker->getRelPosition()));
    return selected;
}

void Timeline::selectFrame(uint index)
{
    // reposition highlighter if neccessary
    if (index < m_zoomTimeline->getFirstIndex() || m_zoomTimeline->getLastIndex() < index) {
        int nHighlighterPos = m_totalTimeline->indexToRelPos(index) - m_highlighter->width() / 2;
        m_highlighter->setRelPosition(nHighlighterPos);
        m_zoomTimeline->updateTimelinelabel(&m_keyframes, getHighlighterRange(), true, m_boundaries);
    }

    // repostion marker
    float newRelPos = m_zoomTimeline->indexToRelPos(index);
    m_marker->setRelPosition(newRelPos);

    m_indexSpinBox->blockSignals(true);
    m_indexSpinBox->setValue(index);
    m_indexSpinBox->blockSignals(false);
}

void Timeline::updateHighlighterWidth()
{
    float relativeWidth = (float) ui->spinBox_zoom->value() / 100;

    // change label width
    Q_ASSERT(relativeWidth <= 1);
    m_highlighter->setWidth(m_totalTimeline->width() * relativeWidth);
    uint minX_Highlighter = m_totalTimeline->parentWidget()->x() + m_totalTimeline->x();
    uint maxX_Highlighter = minX_Highlighter + m_totalTimeline->width() - m_highlighter->width();
    m_highlighter->setIntervall(QPoint(minX_Highlighter, maxX_Highlighter));

    // correct postioning so that the highlighter moves to the left if it would grow over the right edge
    int missPlacement = maxX_Highlighter - m_highlighter->getIntervall().y();
    m_highlighter->setRelPosition(m_highlighter->x() - missPlacement);
    m_zoomTimeline->updateTimelinelabel(&m_keyframes, getHighlighterRange(), true, m_boundaries);
}

void Timeline::resizeEvent(QResizeEvent *ev)
{
    (void) *ev;

    if (m_enableWindow) {
        resize();
    } else {
        // default timeline
        m_totalTimeline->updateTimelinelabel(&this->m_keyframes, QPointF(0, 0), false);
        m_zoomTimeline->updateTimelinelabel(&this->m_keyframes, QPointF(0, 0), true);
    }
}

void Timeline::setEnabled(bool enable)
{
    m_enableWindow = enable;

    // disable all elements
    m_indexSpinBox->setDisabled(!enable);
    m_zoomSpinBox->setDisabled(!enable);
    m_totalTimeline->setDisabled(!enable);
    m_zoomTimeline->setDisabled(!enable);
    m_highlighter->setDisabled(!enable);
    m_marker->setDisabled(!enable);
    m_highlighter->setVisible(enable);
    m_marker->setVisible(enable);
    m_startBoundaryLabel->setVisible(enable);
    m_endBoundaryLabel->setVisible(enable);

    // position boundaries
    if (enable) {
        positionBoundaries(0, m_frameCount - 1);
    }

    // disabled values
    if (!enable) {
        // set spinBox values
        m_indexSpinBox->setValue(0);
        m_zoomSpinBox->setValue(10);
        // display grey timelines
        m_totalTimeline->updateTimelinelabel(&this->m_keyframes, QPointF(0, 42), false);
        m_zoomTimeline->updateTimelinelabel(&this->m_keyframes, QPointF(0, 42), true);
    }
}

QPoint Timeline::getBoundaries()
{
    return m_boundaries;
}

void Timeline::setBoundaries(QPoint boundaries)
{
    positionBoundaries(boundaries.x(), boundaries.y());
}

void Timeline::resetBoundaries()
{
    positionBoundaries(0, m_frameCount - 1);
}

QPointF Timeline::getHighlighterRange()
{
    float firstIndex = m_totalTimeline->relPosToIndex(m_highlighter->getRelPosition());
    float lastIndex = m_totalTimeline->relPosToIndex(m_highlighter->getRelPosition() + m_highlighter->width());
    return QPointF(firstIndex, lastIndex);
}

void Timeline::setupSpinBoxes()
{
    // setup zoomSpinBox
    m_zoomSpinBox->setMinimum(0.1);
    float maxZoom = (float)(50 * m_zoomTimeline->width()) / m_frameCount;
    maxZoom = maxZoom > 50 ? 50 : maxZoom;
    m_zoomSpinBox->setMaximum(maxZoom);
    m_zoomSpinBox->setValue(maxZoom < 10 ? maxZoom : 10);
    // setup indexSpinBox
    m_indexSpinBox->setMinimum(0);
    m_indexSpinBox->setMaximum(m_frameCount - 1);
    m_indexSpinBox->setValue(m_frameCount / 2);
}

/*
 * 1---2
 * |   |
 * 5   3
 *  \ /
 *   4
 */
QPixmap Timeline::drawMarker(uint pixWidth, uint pixHeight, uint symbolWidth, uint symbolHeigth, uint topMargin)
{
    QPixmap pix = QPixmap(pixWidth, pixHeight);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    QPen pen(MARKER_COLOR);
    painter.setPen(pen);
    QPolygon symbol;
    QPoint topLeft = QPoint(pixWidth / 2 - symbolWidth / 2, topMargin);
    symbol << topLeft << QPoint(topLeft.x() + symbolWidth, topLeft.y())
           << QPoint(topLeft.x() + symbolWidth, topLeft.y() + symbolHeigth - symbolWidth) << QPoint(topLeft.x() + symbolWidth / 2, topLeft.y() + symbolHeigth)
           << QPoint(topLeft.x(), topLeft.y() + symbolHeigth - symbolWidth);
    painter.drawPolygon(symbol, Qt::OddEvenFill);
    return pix;
}

/*
 *
 * O-O      O-O
 * |          |
 * |          |
 * O-O      O-O
 *
 */
QPixmap Timeline::drawBoundary(uint pixWidth, uint pixHeight, uint symbolWidth, uint topBottomMargin, bool isStart)
{
    // create transparent pixmap
    QPixmap pix = QPixmap(pixWidth, pixHeight);
    pix.fill(Qt::transparent);
    // create painter and pen with fitting color
    QPainter painter(&pix);
    QPen pen(BOUNDARY_COLOR);
    pen.setWidth(2);
    painter.setPen(pen);

    // set lines which are used to draw the symbol
    uint xMiddle = pixWidth / 2 + pen.width();
    QLine verticalLine = QLine(xMiddle, topBottomMargin, xMiddle, pixHeight - topBottomMargin);
    QPoint topOutterPoint;
    QPoint bottomOutterPoint;
    if (isStart) {
        // bracket is open to the right side
        topOutterPoint = QPoint(xMiddle + symbolWidth, topBottomMargin);
        bottomOutterPoint = QPoint(xMiddle + symbolWidth, pixHeight - topBottomMargin);
    } else {
        // bracket is open to the left side
        topOutterPoint = QPoint(xMiddle - symbolWidth, topBottomMargin);
        bottomOutterPoint = QPoint(xMiddle - symbolWidth, pixHeight - topBottomMargin);
    }
    QLine topHorizontalLine = QLine(QPoint(xMiddle, topBottomMargin), topOutterPoint);
    QLine bottomHorizontalLine = QLine(QPoint(xMiddle, pixHeight - topBottomMargin), bottomOutterPoint);

    painter.drawLine(verticalLine);
    painter.drawLine(bottomHorizontalLine);
    painter.drawLine(topHorizontalLine);
    return pix;
}

void Timeline::positionBoundaries(uint startPos, uint endPos)
{
    // correct parameters if neccessary;
    if (endPos < startPos) {
        endPos = startPos;
    }
    if (startPos > endPos) {
        startPos = endPos;
    }

    uint relPosStart = m_totalTimeline->indexToRelPos(startPos);
    uint relPosEnd = m_totalTimeline->indexToRelPos(endPos);

    // postion boundary labels
    m_startBoundaryLabel->setRelPosition(relPosStart);
    m_endBoundaryLabel->setRelPosition(relPosEnd);

    m_totalTimeline->redraw(QPoint(startPos, endPos));
    m_zoomTimeline->redraw(QPoint(startPos, endPos));

    // update boundaries attribute
    m_boundaries = QPoint(startPos, endPos);
    emit sig_boundariesChanged(m_boundaries);
}

void Timeline::highlighterMoved(int xMovement)
{
    // move highlighter
    int movedPostion = m_highlighter->getRelPosition() + xMovement - m_highlighter->width() / 2;
    m_highlighter->setRelPosition(movedPostion);

    m_zoomTimeline->updateTimelinelabel(&m_keyframes, getHighlighterRange(), true, m_boundaries);

    // reselect currently selected frame to set marker correctly
    uint currentlySelectedIdx = selectedFrame();
    selectFrame(currentlySelectedIdx);

    // only update spinBox
    m_indexSpinBox->blockSignals(true);
    m_indexSpinBox->setValue(currentlySelectedIdx);
    m_indexSpinBox->blockSignals(false);

    emit sig_selectedChanged(currentlySelectedIdx);
}

void Timeline::markerMoved(int xMovement)
{
    // move only in steps
    int movedMarkerPos = m_marker->getRelPosition() + xMovement - m_marker->width() / 2;
    if (movedMarkerPos < 0 || movedMarkerPos >= m_zoomTimeline->width()) {
        return;
    }
    //      calculate new rel position with transformation to index
    float accurateIndex = m_zoomTimeline->relPosToIndex(movedMarkerPos);
    float roundedIndex = qRound(accurateIndex);
    uint correctedRelPos = m_zoomTimeline->indexToRelPos(roundedIndex);
    m_marker->setRelPosition(correctedRelPos);

    m_indexSpinBox->blockSignals(true);
    m_indexSpinBox->setValue(roundedIndex);
    m_indexSpinBox->blockSignals(false);

    emit sig_selectedChanged(roundedIndex);
}

void Timeline::zoomChanged()
{
    updateHighlighterWidth();

    // only update spinBox
    m_indexSpinBox->blockSignals(true);
    m_indexSpinBox->setValue(selectedFrame());
    m_indexSpinBox->blockSignals(false);

    emit sig_selectedChanged(selectedFrame());
}

void Timeline::sbIndexChanged(int index)
{
    selectFrame(index);
    emit sig_selectedChanged(index);
}

void Timeline::boundaryMoved(int xMovement, SlideableLabel *boundaryLabel)
{
    int movedRelPos = boundaryLabel->getRelPosition() + xMovement - boundaryLabel->width() / 2;
    int currIndex = qRound(m_totalTimeline->relPosToIndex(movedRelPos));

    if (boundaryLabel == m_startBoundaryLabel) {
        m_boundaries.setX(currIndex);
    } else if (boundaryLabel == m_endBoundaryLabel) {
        m_boundaries.setY(currIndex);
    }
    // prevent boundaries form crossing over each other
    if (m_boundaries.x() > m_boundaries.y()) {
        // correct last movement
        positionBoundaries(m_boundaries.y(), m_boundaries.y());
    }
    positionBoundaries(m_boundaries.x(), m_boundaries.y());
}

void Timeline::slot_totalTimelineClicked(QPoint pos)
{
    int xMovement = pos.x() - m_highlighter->getRelPosition();
    highlighterMoved(xMovement);
}

void Timeline::slot_zoomTimelineClicked(QPoint pos)
{
    int xMovement = pos.x() - m_marker->getRelPosition();
    markerMoved(xMovement);
}
