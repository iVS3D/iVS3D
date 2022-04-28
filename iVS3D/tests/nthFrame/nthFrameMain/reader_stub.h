#ifndef READER_STUB_H
#define READER_STUB_H


#include <reader.h>

class Reader_stub : public Reader
{
public:
    Reader_stub(unsigned int picCount, double fps);
    cv::Mat getPic(unsigned int, bool = false) { throw "not implemented"; };
    unsigned int getPicCount();
    QString getInputPath() { throw "not implemented"; };
    double getFPS();
    double getVideoDuration() { throw "not implemented"; };
    virtual bool isDir() { throw "not implemented"; };
    virtual Reader *copy() { throw "not implemented"; };
    virtual std::vector<std::string> getFileVector() { throw "not implemented"; };
    virtual void addMetaData(MetaData*) { throw "not implemented"; };
    virtual MetaData* getMetaData() { throw "not implemented"; };

private:
    const uint m_picCount;
    const double m_fps;
};

#endif // READER_STUB_H
