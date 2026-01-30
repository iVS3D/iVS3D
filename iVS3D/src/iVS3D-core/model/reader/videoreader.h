#ifndef VIDEOREADER_H
#define VIDEOREADER_H

#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QFileInfo>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "reader.h"
#include "readerfactory.h"
#include "readerparams.h"
#include "sequentialreaderimpl.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
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

class VideoReader : public Reader

{
   public:
    /**
     * @brief VideoReader constructor which reads the given file and creates
     * cv::VideoCapture from it.
     *
     * @param path Path to the video. Video can be the types, which
     * cv::VideoCapture can handle
     */
    explicit VideoReader(const QString& path,
                         std::shared_ptr<ReaderParams> readerParams);
    /**
     * @brief VideoReader destructor
     *
     */
    ~VideoReader();
    /**
     * @brief Returns the frame to a given index
     *
     * @param index Index of the frame to be returned
     * @return cv::Mat of the selected frame
     */
    cv::Mat getPic(unsigned int index,
                   PictureProcessingFlags flags = APPLY_ALL) override;
    /**
     * @brief Returns the number of frame
     *
     * @return Number of frame
     */
    unsigned int getPicCount() override;
    /**
     * @brief Returns the input path
     *
     * @return Qstring with the input path
     */
    QString getInputPath() override;
    /**
     * @brief Returns the video FPS
     *
     * @return double with the FPS, -1 if input isn't a video
     */
    double getFPS() override;
    /**
     * @brief Returns the video duration
     *
     * @return double with the video duration, -1 if input isn't a video
     */
    double getVideoDuration() override;
    /**
     * @brief Returns wether the input is a direcory or not
     *
     * @return @a true if the input is based on a directory, @a false otherwise
     */
    bool isDir() override;
    /**
     * @brief Creates this reader again and returns it
     *
     * @return New instance of this reader
     */
    VideoReader *copy(std::shared_ptr<ReaderParams> params) override;

    /**
     * @brief Returns a empty vector
     *
     * @return empty vector
     */
    std::vector<std::string> getFileVector() override;

    /**
     * @brief createSequentialReader Creates a SequentialReader object for the
     * given indices. This can be used to access images concurrently if the
     * images are known a priori.
     * @param indices The indices of the images that will be accessed
     * @return SequentialReader instance (caller takes ownership!)
     */
    SequentialReader* createSequentialReader(
        std::vector<uint> indices,
        PictureProcessingFlags flags = APPLY_ALL) override;

    /**
     * @brief addMetaData Used to add MetaData to the reader
     * @param md The MetaData to be saved
     */
    void addMetaData(MetaData* md) override;
    /**
     * @brief getMetaData Returns the currently saved MetaData
     * @return The currently saved MetaData
     */
    MetaData* getMetaData() override;
    /**
     * @brief isValid Retruns wether the reader is valid or not
     * @return @a true if the reader is valid, @a false otherwise
     */
    bool isValid() override;

   private:
    // getter exposed members
    int m_currentIndex = -1;
    std::string m_path = "";
    size_t m_frameCount = -1;
    bool m_isValid = false;
    AVRational m_avgVideoFPS;
    MetaData* m_md = nullptr;
    //

    QMutex m_mutex;
    std::shared_ptr<ReaderParams> m_readerParams;
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
    cv::Mat avFrame2CvMat(const AVFrame* av_f);
    int openFormatContext();
    int selectVideoStream();
    int openCodec();
    int updateSWSContext(const int width, const int height, AVPixelFormat pixFormat);
    int decodeNextPkg(std::vector<int> &decodedIdx);
};

REGISTER_READER("VideoReader", VideoReader)

#endif  // VIDEOREADER_H
