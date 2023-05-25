#include "sequentialreaderimpl.h"

SequentialReaderImpl::SequentialReaderImpl(Reader *reader, std::vector<uint> indices) : m_reader(reader), m_indices(indices), m_idx(0)
{

}


SequentialReaderImpl::SequentialReaderImpl(SequentialReaderImpl &other) : m_reader(other.m_reader), m_indices(other.m_indices), m_idx(other.m_idx)
{

}

bool SequentialReaderImpl::getNext(cv::Mat &image, uint &idx, int &progress)
{
    QMutexLocker locker(&m_mutex);
    if(m_idx >= m_indices.size())
        return false;

    idx = m_indices[m_idx];
    image = m_reader->getPic(idx);
    progress = 100 * m_idx / int(m_indices.size());

    m_idx++;
    return true;
}

uint SequentialReaderImpl::getImageCount()
{
    return uint(m_indices.size());
}

uint SequentialReaderImpl::getCurrentIndex()
{
    return m_idx;
}
