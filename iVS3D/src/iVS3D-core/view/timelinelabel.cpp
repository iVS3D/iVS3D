#include "timelinelabel.h"

TimelineLabel::TimelineLabel(QWidget *parent) : QLabel(parent)
{
}

void TimelineLabel::adjustTimeline(std::vector<uint> *keyframes, uint firstIndex, uint lastIndex, bool drawTimestamps)
{
    this->m_keyframes = keyframes;
    this->m_firstIndex = firstIndex;
    this->m_lastIndex = lastIndex;
    this->m_drawTimestamps = drawTimestamps;

    redraw();
}

float TimelineLabel::getFrameSize()
{
    return (float)width() / (m_lastIndex - m_firstIndex + 1);
}

uint TimelineLabel::getFirstIndex()
{
    return this->m_firstIndex;
}

uint TimelineLabel::getLastIndex()
{
    return this->m_lastIndex;
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
    QPixmap pix = QPixmap(this->geometry().width() - this->lineWidth() - 1, this->geometry().height() - lineWidth() - 1);
    pix.fill(Qt::transparent);
    // setting up paint tool
    QPainter painter(&pix);
    QPen pen;
    pen.setWidth(1);

    // painting
    uint lineHeight = this->geometry().height() * 0.70F;
    uint lineMarginBottom = (this->geometry().height() - lineHeight) / 2;
    uint lineMarginTop = lineMarginBottom;

    // draw timestamps
    if (timeStamps) {
        pen.setColor(Qt::black);
        painter.setPen(pen);
        QVector<QLine> stampLines;
        for (uint i = m_firstIndex; i <= m_lastIndex; i++) {
            uint x = getFrameSize() * (i - m_firstIndex);
            QLine line = QLine(x, this->geometry().height(), x, this->geometry().height() * 0.8);
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
        int x = getFrameSize() * (index - m_firstIndex);
        Q_ASSERT(x < width() && x >= 0);
        QLine line = QLine(x, lineMarginTop, x, lineHeight + lineMarginBottom);
        keyframeLines.push_back(line);
    }
    painter.drawLines(keyframeLines);

    return pix;
}
