#include "concurrentreader.h"

ConcurrentReader::ConcurrentReader(Reader *reader) : m_reader(reader), m_next_idx(UINT_MAX), m_timer(nullptr)
{
}

ConcurrentReader::~ConcurrentReader()
{
    if(m_timer){
        m_timer->stop();
        disconnect(m_timer, &QTimer::timeout, this, &ConcurrentReader::slot_pull);
        delete m_timer;
    }
}

void ConcurrentReader::slot_read(uint idx)
{
    m_next_idx = idx;
    if(m_timer == nullptr){
        m_timer = new QTimer;
        connect(m_timer, &QTimer::timeout, this, &ConcurrentReader::slot_pull);
        m_timer->start(0);
    }
}

void ConcurrentReader::slot_pull()
{
    if(m_next_idx==UINT_MAX){
        return;
    }
    cv::Mat img = m_reader->getPic(m_next_idx);
    emit sig_imageReady(m_next_idx, img);
    m_next_idx = UINT_MAX;
}
