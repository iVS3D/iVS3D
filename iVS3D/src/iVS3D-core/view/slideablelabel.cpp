#include "slideablelabel.h"

SlideableLabel::SlideableLabel(QWidget *parent) : QLabel(parent)
{
    setIntervall(m_xIntervall);
}

void SlideableLabel::mouseMoveEvent(QMouseEvent *ev)
{
    int newX = getRelPosition();
    int delta = ev->x() - width() / 2;
    if (m_stepsEnabled) {
        newX += (int)(delta / m_stepSize) * m_stepSize;
    } else {
        newX += delta;
    }

    setRelPosition(newX);
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
    // calculates the new positon (uses steps if they are enabled)
    float nX = m_xIntervall.x() + delta;

    // check bounderies
    if (nX < m_xIntervall.x()) {
        nX = m_xIntervall.x();
    } else if (m_xIntervall.y() < nX) {
        nX = m_xIntervall.y();
    }

    this->setGeometry(nX , geometry().y(), geometry().width(), geometry().height());
    emit slided();
}

uint SlideableLabel::getRelPosition()
{
    return x() - m_xIntervall.x();
}

void SlideableLabel::setStepSize(float stepSize)
{
    m_stepSize = stepSize;
}

float SlideableLabel::getStepSize()
{
    return m_stepSize;
}

void SlideableLabel::enableSteps(bool enable)
{
    m_stepsEnabled = enable;
}
