#pragma once

#include <map>
#include <opencv2/core/types.hpp>

#include "metadata.h"
#include "reader.h"
#include "readererror.h"

/**
 * @ingroup Reader
 *
 * @class ReaderImpl
 *
 * @brief Contains the implementation of base reader functionality.
 *
 * @details
 * Holds implementations for general functionality used by every reader.
 * These functions do not need to be reimplemented in the concrete reader
 * implentations. Specific Readers derive from this base implementation and not
 * from the iReader interface.
 *
 * @author Dominic Zahn (dominic.zahn@iosb.fraunhofer.de)
 * @date February 2026
 */
class ReaderImpl : public iReader {
   public:
    // image retrieve
    ReaderResult getImage(uint index, cv::Mat outImg,
                          bool postProcessing = true) override;

    /**
     * Wrapper for the specific reader image retrieving logic. It also adds the
     * following:
     * 1. post post processing
     * 2. asynchrounous function call
     * 3. High-Level error detection
     */
    ReaderResult requestImage(uint index,
                              std::future<RetrieveResult>& outFuture,
                              bool postProcessing = true) override;

    // getter
    uint getImageCount() const override { return m_imageCount; }
    std::string getInputPath() const override { return m_filePath; }
    double getDuration() const override { return m_duration; }
    double getAvgFps() const override { return m_avgFps; }

    std::pair<uint, uint> getResolution(
        bool postProcessing = true) const override {
        return m_resolution;
    };
    cv::Rect2f getRoi(bool postProcessing = true) const override {
        return m_roi;
    };

    ReaderResult setResolution(std::pair<uint, uint> resolution) override;
    ReaderResult setRoi(cv::Rect2f) override;

    // TO BE REMOVED
    MetaData* getMetaData() { return m_metaData; }
    void addMetaData(MetaData* md) override { m_metaData = md; }

   protected:
    ReaderResult init(std::string filePath) override;
    /**
     * Abstract function that actually implemnts the specific reader and image
     * retrieving logic.
     */
    virtual RetrieveResult retrieveImage(uint index) = 0;

    // member variables
    std::string m_filePath = "";
    std::mutex m_mutex;
    std::pair<uint, uint> m_resolution = {-1, -1};
    cv::Rect2f m_roi = {-1, -1, -1, -1};
    uint m_imageCount = -1;
    double m_duration = -1;
    double m_avgFps = -1;
    std::map<uint, cv::Mat> m_buffer = {};
    std::map<uint, ReaderResult> m_errorProneIndices = {};

    // TO BE REMOVED
    MetaData* m_metaData = nullptr;
};

#define OUTOFBOUND_GUARD(i) \
    if (0 > i && i >= m_imageCount) return ReaderResult::OutOfBound;
