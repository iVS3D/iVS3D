#ifndef READER_STUB_H
#define READER_STUB_H

#include <reader.h>
#include <opencv2/imgcodecs.hpp>
#include <QMutex>
#include <QMutexLocker>
#include <QtTest>

/**
 *
 * @brief Reader_stub reader stub which takes a List of images and their paths to implement the getPic and getPicCount method
 *
 * @author Daniel Brommer
 *
 * @date 2021/04/14
 */

class Reader_stub : public Reader
{
public:
    Reader_stub(std::vector<uint> picOrder, std::vector<QString> picPaths);
    cv::Mat getPic(unsigned int);
    unsigned int getPicCount();
    QString getInputPath() { throw "not implemented"; };
    double getFPS() { throw "not implemented"; };
    double getVideoDuration() { throw "not implemented"; };
    virtual bool isDir() { throw "not implemented"; };
    virtual Reader *copy() { throw "not implemented"; };
    virtual bool isValid() { throw "not implemented"; };
    virtual std::vector<std::string> getFileVector() { throw "not implemented"; };
    virtual void addMetaData(MetaData*) { throw "not implemented"; };
    virtual MetaData* getMetaData() { throw "not implemented"; };
    SequentialReader *createSequentialReader(std::vector<uint> indices);

private:
    std::vector<uint> m_picOrder;
    std::vector<QString> m_picPaths;
    uint m_picCount;
    QMutex m_mutex;
};

#endif // READER_STUB_H
