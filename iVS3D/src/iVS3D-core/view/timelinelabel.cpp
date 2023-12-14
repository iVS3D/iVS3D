#include "timelinelabel.h"

TimelineLabel::TimelineLabel(QWidget *parent) : QLabel(parent)
{
}

void TimelineLabel::updateTimelinelabel(std::vector<uint> *keyframes, QPointF indexBounds, bool drawTimestamps, QPoint boundaries)
{
    if (!keyframes) {
        return;
    }

    m_firstIndex = qCeil(indexBounds.x());
    m_lastIndex = qFloor(indexBounds.y());
    m_frameSize = indexBounds.x() <= indexBounds.y() ? (float)width() / (indexBounds.y() - indexBounds.x()) : width();
    m_firstIndex_offset = m_frameSize * (m_firstIndex - indexBounds.x());
    m_lastIndex_offset = m_frameSize * (indexBounds.y() - m_lastIndex);

    m_keyframes = keyframes;
    m_drawTimestamps = drawTimestamps;

    redraw(boundaries);
}

float TimelineLabel::relPosToIndex(int relativePosition)
{
    float index = m_firstIndex + (float)(relativePosition - m_firstIndex_offset) / m_frameSize;

    if (index < 0)
        return 0.f;
    else if (index > m_lastIndex + 1)
        return float(m_lastIndex);

    return index;
}

uint TimelineLabel::indexToRelPos(float index)
{
    if (index < m_firstIndex) {
        return 0;
    }
    if (index > m_lastIndex) {
        return width() - 1;
    }

    int relPos = m_frameSize * (index - m_firstIndex) + m_firstIndex_offset;
    if (relPos >= width())
        return width() - m_lastIndex_offset;

    return (uint)relPos;
}

uint TimelineLabel::getFirstIndex()
{
    return m_firstIndex;
}

uint TimelineLabel::getLastIndex()
{
    return m_lastIndex;
}

void TimelineLabel::redraw(QPoint boundaries)
{
    setPixmap(drawPixmap(m_drawTimestamps, boundaries));
}

void TimelineLabel::mousePressEvent(QMouseEvent *ev)
{
    emit sig_clicked(ev->pos());
}

QPixmap TimelineLabel::drawPixmap(bool timeStamps, QPoint boundaries)
{
    QPixmap pix = QPixmap(geometry().width() - lineWidth() - 1, geometry().height() - lineWidth() - 1);
    pix.fill(Qt::transparent);
    // setting up paint tool
    QPainter painter(&pix);
    QPen pen;
    pen.setWidth(1);

    // painting
    uint lineHeight = geometry().height() * 0.70F;
    uint lineMarginBottom = (geometry().height() - lineHeight) / 2;
    uint lineMarginTop = lineMarginBottom;

    // draw timestamps
    if (timeStamps) {
        pen.setColor(TIMESTAMP_COLOR);
        painter.setPen(pen);
        QVector<QLine> stampLines;
        for (uint i = m_firstIndex; i <= m_lastIndex; i++) {
            uint x = indexToRelPos(i);
            QLine line = QLine(x, geometry().height(), x, geometry().height() * 0.8);
            stampLines.push_back(line);
        }
        painter.drawLines(stampLines);
    }

    // draw keyframes
    QVector<QLine> inBoundKeyframeLines, outOfBoundKeyframeLines;
    uint lowerBound = boundaries.x() < 0 ? m_firstIndex : (uint)boundaries.x();
    uint upperBound = boundaries.y() < 0 ? m_lastIndex : (uint)boundaries.y();
    foreach (uint index, *m_keyframes) {
        if (index < m_firstIndex) {
            continue;
        }
        if (index > m_lastIndex) {
            break;
        }

        uint x = indexToRelPos(index);
        if (lowerBound <= index && index <= upperBound) {
            QLine line = QLine(x, lineMarginTop, x, lineHeight + lineMarginBottom);
            inBoundKeyframeLines.push_back(line);
        } else {
            QLine line = QLine(x, lineMarginTop + lineHeight / 3, x, lineHeight + lineMarginBottom);
            outOfBoundKeyframeLines.push_back(line);
        }
    }
    // in bound keyframes
    pen.setColor(INBOUND_COLOR);
    painter.setPen(pen);
    painter.drawLines(inBoundKeyframeLines);
    // out of bound keyframes
    pen.setColor(OUTOFBOUND_COLOR);
    painter.setPen(pen);
    painter.drawLines(outOfBoundKeyframeLines);

    // draw boundaries
    if (timeStamps) {
        pen.setColor(BOUNDARY_COLOR);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLines(boundaryLines(boundaries));
    }
    return pix;
}

QVector<QLine> TimelineLabel::boundaryLines(QPoint boundaries)
{
    QVector<QLine> lines;
    uint labelHeight = this->height();
    uint xOffsetLowerBound = indexToRelPos(boundaries.x());
    uint xOffsetUpperBound = indexToRelPos(boundaries.y());

    lines.push_back(QLine(QPoint(xOffsetLowerBound, 0),
                          QPoint(xOffsetLowerBound, labelHeight - 1)));
    lines.push_back(QLine(QPoint(xOffsetUpperBound, 0),
                          QPoint(xOffsetUpperBound, labelHeight - 1)));
    return lines;
}
