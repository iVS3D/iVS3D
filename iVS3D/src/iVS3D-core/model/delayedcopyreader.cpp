#include "delayedcopyreader.h"
#include "delayedcopyreader.h"

#include <QDebug>

DelayedCopyReader::DelayedCopyReader(Reader *reader)
{
    m_realReader = reader;
    m_copyReader = nullptr;
    qDebug() << "create copy reader";
}

cv::Mat DelayedCopyReader::getPic(unsigned int idx, bool)
{
    if(!m_copyReader){
        m_copyReader = m_realReader->copy();
        qDebug() << "copy reader now";
    }
    return m_copyReader->getPic(idx);
}

unsigned int DelayedCopyReader::getPicCount()
{
    if(!m_copyReader){
        return m_realReader->getPicCount();
    }
    return m_copyReader->getPicCount();
}

QString DelayedCopyReader::getInputPath()
{
    if(!m_copyReader){
        return m_realReader->getInputPath();
    }
    return m_copyReader->getInputPath();
}

double DelayedCopyReader::getFPS()
{
    if(!m_copyReader){
        return m_realReader->getFPS();
    }
    return m_copyReader->getFPS();
}

double DelayedCopyReader::getVideoDuration()
{
    if(!m_copyReader){
        return m_realReader->getVideoDuration();
    }
    return m_copyReader->getVideoDuration();
}

bool DelayedCopyReader::isDir()
{
    if(!m_copyReader){
        return m_realReader->isDir();
    }
    return m_copyReader->isDir();
}

Reader *DelayedCopyReader::copy()
{
    qDebug() << "copy copy???";
    return new DelayedCopyReader(m_realReader);
}

std::vector<std::string> DelayedCopyReader::getFileVector()
{
    if(!m_copyReader){
        return m_realReader->getFileVector();
    }
    return m_copyReader->getFileVector();
}

void DelayedCopyReader::initMultipleAccess(const std::vector<uint> &frames)
{
    if(!m_copyReader){
        m_realReader->initMultipleAccess(frames);
        return;
    }
    m_copyReader->initMultipleAccess(frames);
}

void DelayedCopyReader::enableMultithreading()
{
    if(!m_copyReader){
        m_copyReader = m_realReader->copy();
        qDebug() << "copy reader now";
    }
}
