#include "timelinelabel.h"

TimelineLabel::TimelineLabel(QWidget *parent) : QLabel(parent)
{
}

void TimelineLabel::updateTimelinelabel(std::vector<uint> *keyframes, QPointF indexBounds, bool drawTimestamps)
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

    redraw();
}

float TimelineLabel::relPosToIndex(uint relativePosition)
{
    float index = m_firstIndex + (float)(relativePosition - m_firstIndex_offset) / m_frameSize;

    if (index < 0)
        return 0.f;
    else if (index > m_lastIndex + 1)
        return width();

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

void TimelineLabel::redraw()
{
    setPixmap(drawPixmap(m_drawTimestamps));
}

void TimelineLabel::mousePressEvent(QMouseEvent *ev)
{
    emit sig_clicked(ev->pos());
}

QPixmap TimelineLabel::drawPixmap(bool timeStamps)
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
        pen.setColor(Qt::black);
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
    pen.setColor(Qt::red);
    painter.setPen(pen);
    QVector<QLine> keyframeLines;
    foreach (uint index, *m_keyframes) {
        if (index < m_firstIndex) {
            continue;
        }
        if (index > m_lastIndex) {
            break;
        }
        uint x = indexToRelPos(index);
        QLine line = QLine(x, lineMarginTop, x, lineHeight + lineMarginBottom);
        keyframeLines.push_back(line);
    }
    painter.drawLines(keyframeLines);

    return pix;
}
