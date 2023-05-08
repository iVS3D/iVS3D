#ifndef SEQUENTIALREADERIMPL_H
#define SEQUENTIALREADERIMPL_H

#include "sequentialreader.h"
#include "reader.h"

class SequentialReaderImpl : public SequentialReader
{
public:
    SequentialReaderImpl(Reader *reader, std::vector<uint> indices);
    SequentialReaderImpl(SequentialReaderImpl &other);

    virtual bool getNext(cv::Mat &image, uint &idx, int &progress) override;

    virtual uint getImageCount() override;
    virtual uint getCurrentIndex() override;

private:
    Reader *m_reader;
    std::vector<uint> m_indices;
    uint m_idx;
    QMutex m_mutex;
};

#endif // SEQUENTIALREADERIMPL_H
