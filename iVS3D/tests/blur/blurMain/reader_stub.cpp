#include "reader_stub.h"

#include <QDebug>

Reader_stub::Reader_stub(std::vector<uint> picOrder, std::vector<QString> picPaths) :
    m_picOrder(picOrder), m_picPaths(picPaths), m_picCount((int)picOrder.size())
{

}

cv::Mat Reader_stub::getPic(uint idx) {
    QMutexLocker locker(&m_mutex);
    int current = m_picOrder.at(idx);
    cv::Mat ret = cv::imread(m_picPaths.at(current).toStdString());
    return ret;
}

uint Reader_stub::getPicCount() {
    return m_picCount;
}
