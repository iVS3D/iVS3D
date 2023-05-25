#include "sequentialreaderimpl.h"

SequentialReaderImpl::SequentialReaderImpl(Reader *reader, std::vector<uint> indices, bool lockConcurrentAccess)
    : m_reader(reader), m_indices(indices), m_idx(0), m_lockConcurrentAccess(lockConcurrentAccess)
{

}


SequentialReaderImpl::SequentialReaderImpl(SequentialReaderImpl &other)
    : m_reader(other.m_reader), m_indices(other.m_indices), m_idx(other.m_idx), m_lockConcurrentAccess(other.m_lockConcurrentAccess)
{

}

bool SequentialReaderImpl::getNext(cv::Mat &image, uint &idx, int &progress)
{
    {
        // this part is protected by the mutex
        QMutexLocker locker(&m_mutex);

        if(m_idx >= m_indices.size())
            return false;

        idx = m_indices[m_idx];
        progress = 100 * m_idx / int(m_indices.size());
        m_idx++;

        if(m_lockConcurrentAccess){
            // the user wants to protect the reader as well
            image = m_reader->getPic(idx);
            return true;
        }
    }

    // this call to the reader is not protected by the mutex on purpose
    image = m_reader->getPic(idx);
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
