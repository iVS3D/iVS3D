#include "timeline.h"

#include <QDebug>

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
    m_marker->setWidth(QGuiApplication::primaryScreen()->size().width() / 10);
    m_marker->setFrameStyle(QFrame::NoFrame);
    m_marker->enableSteps(true);
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

    this->m_totalTimeline = ui->label_totalTimeline;
    this->m_zoomTimeline = ui->label_zoomTimeline;
    this->m_zoomSpinBox = ui->spinBox_zoom;
    this->m_indexSpinBox = ui->spinBox_index;

    // connect slideEvents to the Timeline
    QObject::connect(this->m_highlighter, &SlideableLabel::slided, this, &Timeline::highlighterMoved);
    QObject::connect(this->m_marker, &SlideableLabel::slided, this, &Timeline::markerMoved);

    // connect spinBoxes
    QObject::connect(m_zoomSpinBox, SIGNAL(valueChanged(double)), this, SLOT(zoomChanged()));
    QObject::connect(m_indexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Timeline::sbIndexChanged);

    // connect slideEvents of the moving boundarie labels
    QObject::connect(m_startBoundaryLabel, &SlideableLabel::slided, this, &Timeline::startBoundaryMoved);
    QObject::connect(m_endBoundaryLabel, &SlideableLabel::slided, this, &Timeline::endBoundaryMoved);

    // connect clickEvents of the timelineLabels
    QObject::connect(m_zoomTimeline, &TimelineLabel::sig_clicked, this, &Timeline::slot_zoomTimelineClicked);
    QObject::connect(m_totalTimeline, &TimelineLabel::sig_clicked, this, &Timeline::slot_totalTimelineClicked);

    // setup shortcuts
    m_resetSc = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    QObject::connect(m_resetSc, &QShortcut::activated, this, &Timeline::slot_resetSc);

    // disable window
    setEnabled(false);
}

Timeline::~Timeline()
{
    delete ui;
}

void Timeline::updateKeyframes(const std::vector<uint> &newKeyframes)
{
    m_keyframes = newKeyframes;
    m_zoomTimeline->redraw();
    m_totalTimeline->redraw();
}

void Timeline::resize()
{   
    int currentImage = m_indexSpinBox->value();
    // setup highlighter
    m_highlighter->setYLevel(m_totalTimeline->parentWidget()->y());
    m_highlighter->setHeight(m_totalTimeline->parentWidget()->height());
    updateHighlighterWidth();

    // set frames for each timelineLabel
    m_totalTimeline->adjustTimeline(&this->m_keyframes, 0, m_frameCount - 1, false);
    QPoint frameRange = getHighlighterRange();
    m_zoomTimeline->adjustTimeline(&this->m_keyframes, frameRange.x(), frameRange.y(), true);

    // setup marker
    m_marker->setYLevel(m_zoomTimeline->parentWidget()->y());
    m_marker->setHeight(m_zoomTimeline->parentWidget()->height());
    uint markerMinX = m_zoomTimeline->parentWidget()->x() - m_marker->width() / 2 - m_marker->lineWidth() + 3;
    uint markerMaxX = markerMinX + m_zoomTimeline->width();
    m_marker->setIntervall(QPoint(markerMinX, markerMaxX));
//    // set bounderies for the second time to adjust the maxX
    markerMaxX = markerMinX + (int)(m_zoomTimeline->getFrameSize() * (m_zoomTimeline->getLastIndex() - m_zoomTimeline->getFirstIndex()));
    m_marker->setStepSize(m_zoomTimeline->getFrameSize());
    m_marker->enableSteps(true);
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
    m_boundaries = QPoint(0, 0);

    resize();

    // reset boundaries to min and max postion
    positionBoundaries(0, m_frameCount -1);

    setupSpinBoxes();
}

uint Timeline::selectedFrame()
{
    float selected = round((float)m_marker->getRelPosition() / m_marker->getStepSize() + m_zoomTimeline->getFirstIndex());

    // prevent marker from moving to a out of bounds frame
    if (selected < m_zoomTimeline->getFirstIndex()) {
        selected = m_zoomTimeline->getFirstIndex();
    } else if (m_zoomTimeline->getLastIndex() < selected) {
        selected = m_zoomTimeline->getLastIndex();
    }
    return selected;
}

void Timeline::selectFrame(uint index)
{
    // reposition highlighter if neccessary
    QPoint highlighterRange = getHighlighterRange();
    if (index < (uint)highlighterRange.x() || (uint)highlighterRange.y() < index) {
        int nHighlighterPos = m_totalTimeline->getFrameSize() * index - m_highlighter->width() / 2;
        m_highlighter->setRelPosition(nHighlighterPos);
    }

    // repostion marker
    float newRelPos = m_zoomTimeline->getFrameSize() * (index - m_zoomTimeline->getFirstIndex());
    m_marker->setRelPosition(newRelPos);
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

    // adjust filling
    QPoint highlighterRange = getHighlighterRange();
    m_zoomTimeline->adjustTimeline(&m_keyframes, highlighterRange.x(), highlighterRange.y(), true);

    // adjust Marker intervall
    uint minX_Marker = m_marker->getIntervall().x();
    uint maxX_Marker = minX_Marker + m_zoomTimeline->width();
    m_marker->setStepSize(m_zoomTimeline->getFrameSize());
    m_marker->setIntervall(QPoint(minX_Marker, maxX_Marker));
    maxX_Marker = minX_Marker + (int)(m_zoomTimeline->getFrameSize() * (m_zoomTimeline->getLastIndex() - m_zoomTimeline->getFirstIndex()));
    m_marker->setIntervall(QPoint(minX_Marker, maxX_Marker));
}

void Timeline::resizeEvent(QResizeEvent *ev)
{
    (void) *ev;

    if (m_enableWindow) {
        resize();
    } else {
        // default timeline
        m_totalTimeline->adjustTimeline(&this->m_keyframes, 0, 42, false);
        m_zoomTimeline->adjustTimeline(&this->m_keyframes, 0, 42, true);
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

    // display grey timelines
    m_totalTimeline->adjustTimeline(&this->m_keyframes, 0, 42, false);
    m_zoomTimeline->adjustTimeline(&this->m_keyframes, 0, 42, true);

    // set spinBox values
    m_indexSpinBox->setValue(0);
    m_zoomSpinBox->setValue(10);

    // position boundaries
    if (enable) {
        positionBoundaries(0, m_frameCount - 1);
    }
}

QPoint Timeline::getBoundaries()
{
    return m_boundaries;
}

QPoint Timeline::getHighlighterRange()
{
    uint firstIndex = (m_highlighter->geometry().x() - m_highlighter->getIntervall().x()) / m_totalTimeline->getFrameSize();
    return QPoint(firstIndex, firstIndex + m_frameCount * ((float) m_zoomSpinBox->value() / 100));
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
    QPen pen(Qt::black);
    painter.setPen(pen);
    QPolygon symbol;
    QPoint topLeft = QPoint(pixWidth / 2 - symbolWidth / 2, topMargin);
    symbol << topLeft << QPoint(topLeft.x() + symbolWidth, topLeft.y())
           << QPoint(topLeft.x() + symbolWidth, topLeft.y() + symbolHeigth - symbolWidth) << QPoint(topLeft.x() + symbolWidth / 2, topLeft.y() + symbolHeigth)
           << QPoint(topLeft.x(), topLeft.y() + symbolHeigth - symbolWidth);
    painter.drawPolygon(symbol, Qt::OddEvenFill);
    return pix;
}

uint Timeline::calcBoundary(SlideableLabel *boundaryLabel)
{
    // calculate size of one frame in the totalTimeline
    uint selectedFrame = (boundaryLabel->getRelPosition() * m_frameCount) / m_totalTimeline->width();
    if (selectedFrame > m_frameCount - 1)
        return m_frameCount - 1;
    else
        return selectedFrame;
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
    QPen pen(Qt::black);
    pen.setWidth(2);
    painter.setPen(pen);

    // set lines which are used to draw the symbol
    uint xMiddle = pixWidth / 2;
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
    Q_ASSERT(startPos < m_frameCount);
    Q_ASSERT(endPos < m_frameCount);

    // disconnect events to prevent a loop
    QObject::disconnect(m_startBoundaryLabel, &SlideableLabel::slided, this, &Timeline::startBoundaryMoved);
    QObject::disconnect(m_endBoundaryLabel, &SlideableLabel::slided, this, &Timeline::endBoundaryMoved);

    uint relPosStart = 0;
    uint relPosEnd = 0;
    // protecting from division with 0
    if (m_frameCount != 0) {
        relPosStart = (startPos * m_totalTimeline->width()) / m_frameCount;
        relPosEnd = (endPos * m_totalTimeline->width()) / m_frameCount;
    }

    // postion boundary labels
    m_startBoundaryLabel->setRelPosition(relPosStart);
    m_endBoundaryLabel->setRelPosition(relPosEnd);

    // update boundaries attribute
    m_boundaries = QPoint(startPos, endPos);

    // reconnect events to prevent a loop
    QObject::connect(m_startBoundaryLabel, &SlideableLabel::slided, this, &Timeline::startBoundaryMoved);
    QObject::connect(m_endBoundaryLabel, &SlideableLabel::slided, this, &Timeline::endBoundaryMoved);
}

void Timeline::highlighterMoved()
{
    QPoint highlighterRange = Timeline::getHighlighterRange();
    m_zoomTimeline->adjustTimeline(&m_keyframes, highlighterRange.x(), highlighterRange.y(), true);
    m_marker->setStepSize(m_zoomTimeline->getFrameSize());

    // only update spinBox
    m_indexSpinBox->setValue(selectedFrame());
}

void Timeline::markerMoved()
{
    m_indexSpinBox->setValue(selectedFrame());
}

void Timeline::zoomChanged()
{
    updateHighlighterWidth();

    // only update spinBox
    m_indexSpinBox->setValue(selectedFrame());
}

void Timeline::sbIndexChanged(int index)
{
    selectFrame(index);
    emit sig_selectedChanged(index);
}

void Timeline::startBoundaryMoved()
{
    m_boundaries.setX(calcBoundary(m_startBoundaryLabel));
    // prevent boundaries form crossing over each other
    if (m_boundaries.x() > m_boundaries.y()) {
        // correct last movement
        positionBoundaries(m_boundaries.y(), m_boundaries.y());
    }
}

void Timeline::endBoundaryMoved()
{
    m_boundaries.setY(calcBoundary(m_endBoundaryLabel));
    // prevent boundaries form crossing over each other
    if (m_boundaries.x() > m_boundaries.y()) {
        // correct last movement
        positionBoundaries(m_boundaries.x(), m_boundaries.x());
    }
}

void Timeline::slot_resetSc()
{
    positionBoundaries(0, m_frameCount - 1);
}

void Timeline::slot_totalTimelineClicked(QPoint pos)
{
    m_highlighter->setRelPosition(pos.x() - m_highlighter->width() / 2);
}

void Timeline::slot_zoomTimelineClicked(QPoint pos)
{
    m_marker->setRelPosition(pos.x());
}
