#pragma once

#include <reader_impl.h>

#include <future>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

#include "postprocessor.hpp"
#include "readererror.h"

ReaderResult ReaderImpl::init(std::string filePath) {
    m_filePath = std::move(filePath);
    return ReaderResult::Success;
}

ReaderResult ReaderImpl::requestImage(uint index,
                                      std::future<RetrieveResult>& outFuture,
                                      bool postProcessing) {
    if (m_errorProneIndices.count(index) > 0) {
        // index was already found to be error prone
        return m_errorProneIndices[index];
    }

    std::pair<uint, uint> res = {-1, -1};
    cv::Rect2f roi = {-1, -1, -1, -1};
    if (postProcessing) {
        res = m_resolution;
        roi = m_roi;
    }
    auto lambda = [&, this](uint index) -> RetrieveResult {
        std::lock_guard<std::mutex> lk(m_mutex);

        // read image with specific implementation
        auto result = this->retrieveImage(index);

        // apply post processing
        bool res_unvalid = res.first < 0 || res.first < 0;
        bool roi_unvalid = roi.area() < 0;
        if (!res_unvalid && !roi_unvalid) {
            cv::Mat img_processed;
            PostProcessor pp = PostProcessor(res, roi);
            result.first = pp.applyAll(result.second, img_processed);
            result.second = img_processed;
        }

        // safe to buffer
        if (result.first == ReaderResult::Success) {
            m_buffer[index] = result.second;
        } else {
            m_errorProneIndices[index] = result.first;
        }
        return result;
    };
    // start in async
    outFuture = std::async(lambda, index);
    return ReaderResult::Success;
}

ReaderResult ReaderImpl::getImage(uint index, cv::Mat outImg,
                                  bool postProcessing) {
    std::future<RetrieveResult> future;
    ReaderResult reader_result = requestImage(index, future, postProcessing);

    future.wait();
    if (reader_result != ReaderResult::Success) return reader_result;

    RetrieveResult future_result = future.get();
    future_result.second.copyTo(outImg);

    return future_result.first;
}

ReaderResult ReaderImpl::setResolution(std::pair<uint, uint> resolution) {
    bool w_valid = resolution.first > 0;
    bool h_valid = resolution.second > 0;
    if (!w_valid || !h_valid) return ReaderResult::PostProcessorError;

    m_resolution = resolution;
    return ReaderResult::Success;
}

ReaderResult ReaderImpl::setRoi(cv::Rect2f roi) {
    bool x_valid = 0 <= roi.x && roi.x <= 1;
    bool y_valid = 0 <= roi.y && roi.y <= 1;
    bool w_valid = 0 <= roi.width && roi.width <= 1 - roi.x;
    bool h_valid = 0 <= roi.height && roi.height <= 1 - roi.y;
    if (!w_valid || !h_valid || !x_valid || !y_valid)
        return ReaderResult::PostProcessorError;

    m_roi = roi;
    return ReaderResult::Success;
}
