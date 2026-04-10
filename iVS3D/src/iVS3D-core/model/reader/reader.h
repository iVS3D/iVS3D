#ifndef READER_H
#define READER_H

#include <future>
#include <opencv2/core/types.hpp>

#include "metadata.h"
#include "readererror.h"

/**
 * @defgroup Reader
 * @ingroup Reader
 *
 * @interface iReader
 *
 * @brief iReader interface for handling and accessing videos or image
 * sequences.
 *
 * @details
 * Defines the high-level interface for readers in general.
 * It allows uniform access to video or images in form a standardizes functions.
 * For video or image specific functiality cast towards the respective  concrete
 * implementation.
 *
 * @author Dominic Zahn
 * @date February 2026
 */

class iReader {
   public:
    typedef std::pair<ReaderResult, cv::Mat> RetrieveResult;
    virtual ~iReader() {};

    // image retrieve
    virtual ReaderResult getImage(uint index, cv::Mat outImg,
                                  bool postProcessing = true) = 0;

    virtual ReaderResult requestImage(uint index,
                                      std::future<RetrieveResult>& outFuture,
                                      bool postProcessing = true) = 0;
    // getter
    virtual uint getImageCount() const = 0;
    virtual std::string getInputPath() const = 0;
    virtual double getDuration() const = 0;
    virtual double getAvgFps() const = 0;
    virtual ReaderResult index2Pts(uint index, double& outPts) const = 0;
    virtual ReaderResult pts2Index(double pts, uint& index) const = 0;
    virtual std::pair<uint, uint> getResolution(
        bool postProcessing = true) const = 0;
    virtual cv::Rect2f getRoi(bool postProcessing = true) const = 0;
    // setter
    virtual ReaderResult setResolution(std::pair<uint, uint> resolution) = 0;
    virtual ReaderResult setRoi(cv::Rect2f roi) = 0;

    // TO BE REMOVED
    virtual void addMetaData(MetaData* metaData) = 0;

   protected:
    //      init
    friend class ReaderFactory;  // to provide init for ReaderFactory
    virtual ReaderResult init(std::string filePath) = 0;
};

#endif  // READER_H
