#ifndef VIDEOREADER_H
#define VIDEOREADER_H

#include <QFileInfo>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "reader_impl.h"
#include "readererror.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
}

/**
 * @class VideoReader
 *
 * @ingroup Model
 *
 * @brief The VideoReader class is used to import video files and implement the
 * Reader interface. It utilizes the FFMPEG libaries to enable frame perfect
 * random access inside a video stream.
 *
 * @author Dominic Zahn
 *
 * @date 2025/08/01
 */

class VideoReader : public ReaderImpl

{
   public:
    explicit VideoReader();

    ~VideoReader() override;

    // image retrieve
    ReaderResult init(std::string filePath) override;

    // getter
    ReaderResult index2Pts(uint index, double& outPts) const override;
    ReaderResult pts2Index(double pts, uint& index) const override;

    static bool suffixValidation(const QFileInfo& fileInfo);

   private:
    // std::pair<ReaderResult, cv::Mat> retrieveImage(uint index) override;
    RetrieveResult retrieveImage(uint index) override;
    // getter exposed members
    int m_currentIndex = -1;
    AVRational m_rational_avgVideoFPS;
    //

    int64_t m_startTimestamp = AV_NOPTS_VALUE;
    std::map<uint, AVFrame*> m_buffer;
    AVRational m_streamTimeBase = AV_TIME_BASE_Q;
    int m_streamId = -1;
    int m_lastFrameIdx = -1;
    // AV/SWS-Objects
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    const AVCodec* m_codec = nullptr;
    struct SwsContext* m_swsContext = nullptr;

    // private helper functions
    bool calcAvgFps();
    bool calcDuration();

    cv::Mat avFrame2CvMat(const AVFrame* av_f);
    int openFormatContext(const std::string& path);
    int selectVideoStream();
    int openCodec();
    int updateSWSContext(const int width, const int height,
                         AVPixelFormat pixFormat);
    int decodeNextPkg(std::vector<int>& decodedIdx);
};

#endif  // VIDEOREADER_H
