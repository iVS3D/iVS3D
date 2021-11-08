#include "reader_stub.h"

Reader_stub::Reader_stub(unsigned int picCount, double fps) : m_picCount(picCount), m_fps(fps) { }

unsigned int Reader_stub::getPicCount()
{
    return m_picCount;
}

double Reader_stub::getFPS()
{
    return m_fps;
}
