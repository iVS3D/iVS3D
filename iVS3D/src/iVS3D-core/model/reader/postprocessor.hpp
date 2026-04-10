#pragma once

#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

#include "readererror.h"

class PostProcessor {
   public:
    PostProcessor(std::pair<uint, uint> resolution, cv::Rect2f roi)
        : m_resolution(resolution), m_roi(roi) {

          };

    ReaderResult resize(const cv::Mat& in, cv::Mat& out) const {
        try {
            cv::Size size = cv::Size(m_resolution.first, m_resolution.second);
            cv::resize(in, out, size, 0, 0, cv::INTER_AREA);
        } catch (const cv::Exception& e) {
            return ReaderResult::PostProcessorError;
        }
        return ReaderResult::Success;
    }

    ReaderResult crop(const cv::Mat& in, cv::Mat& out) const {
        cv::Rect roi_scaled = scaleROI(m_roi);

        try {
            out = in(m_roi);
        } catch (const cv::Exception& e) {
            return ReaderResult::PostProcessorError;
        }
        return ReaderResult::Success;
    }

    ReaderResult applyAll(const cv::Mat& in, cv::Mat& out) const {
        cv::Mat resized;
        ReaderResult r_resize = resize(in, resized);
        if (r_resize != ReaderResult::Success) return r_resize;

        ReaderResult r_crop = crop(resized, out);
        if (r_crop != ReaderResult::Success) return r_crop;

        return ReaderResult::Success;
    }

    std::pair<uint, uint> getResolution() { return m_resolution; }

    cv::Rect2f getROI() { return m_roi; }

   private:
    cv::Rect scaleROI(cv::Rect2f roi_normalized) const {
        return cv::Rect(m_roi.x * m_resolution.first,
                        m_roi.y * m_resolution.second,
                        m_roi.width * m_resolution.first,
                        m_roi.height * m_resolution.second);
    };
    std::pair<uint, uint> m_resolution = {-1, -1};
    cv::Rect2f m_roi = {-1, -1, -1, -1};
};
