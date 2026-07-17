#ifndef SEQUENTIALREADER_H
#define SEQUENTIALREADER_H

#include <stdlib.h>

#include <QMutex>
#include <QMutexLocker>

#include "opencv2/core/mat.hpp"

class SequentialReader {
   public:
    virtual ~SequentialReader() {}
    virtual bool getNext(cv::Mat& image, uint& idx, int& progress) = 0;
    bool getNext(cv::Mat& image, uint& idx) {
        int progress;
        return getNext(image, idx, progress);
    }
    virtual uint getImageCount() = 0;
    virtual uint getCurrentIndex() = 0;
};

#endif  // SEQUENTIALREADER_H
