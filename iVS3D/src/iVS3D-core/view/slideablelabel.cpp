#include "slideablelabel.h"

SlideableLabel::SlideableLabel(QWidget *parent) : QLabel(parent)
{
    setIntervall(m_xIntervall);
}

void SlideableLabel::mouseMoveEvent(QMouseEvent *ev)
{
    emit mouseMoved(ev->x());
}

void SlideableLabel::setIntervall(QPoint xRange)
{
    this->m_xIntervall = xRange;
}

QPoint SlideableLabel::getIntervall()
{
    return m_xIntervall;
}

void SlideableLabel::setYLevel(int y)
{
    this->setGeometry(geometry().x(), y, geometry().width(), geometry().height());
}

void SlideableLabel::setHeight(uint height)
{
    this->setGeometry(geometry().x(), geometry().y(), geometry().width(), height);
}

void SlideableLabel::setWidth(uint width)
{
    this->setGeometry(geometry().x(), geometry().y(), width, geometry().height());
}

void SlideableLabel::setRelPosition(float delta)
{
    float nX = m_xIntervall.x() + delta;

    // check bounderies
    if (nX < m_xIntervall.x()) {
        nX = m_xIntervall.x();
    } else if (m_xIntervall.y() < nX) {
        nX = m_xIntervall.y();
    }

    this->setGeometry(nX , geometry().y(), geometry().width(), geometry().height());
}

uint SlideableLabel::getRelPosition()
{
    int nPos = x() - m_xIntervall.x();
    return nPos < 0 ? 0 : uint(nPos);
}
