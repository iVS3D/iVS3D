#include "videoreader.h"

#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>

#include <cstdint>
#include <string>
#include <vector>

#include "reader_impl.h"
#include "readererror.h"

bool VideoReader::suffixValidation(const QFileInfo& fileInfo) {
    QStringList validSuffixList = {".mp4", ".MP4", ".mov", ".MOV"};
    for (QString suffix : validSuffixList) {
        if (suffix == fileInfo.suffix()) return true;
    }
    return false;
}

ReaderResult VideoReader::init(std::string filePath) {
    ReaderResult parentInit = ReaderImpl::init(filePath);
    if (parentInit != ReaderResult::Success) return parentInit;

    if (openFormatContext(filePath) > 0) return ReaderResult::InitError;
    if (selectVideoStream() > 0) return ReaderResult::InitError;
    if (openCodec() > 0) return ReaderResult::InitError;

    // validate the given readerParams, initialize if necessary!
    uint w = m_codecContext->width;
    uint h = m_codecContext->height;

    if (w < 10 || h < 10) {
        return ReaderResult::InitError;
    }

    if (!calcAvgFps()) return ReaderResult::InitError;
    if (!calcDuration()) return ReaderResult::InitError;

    // reduce framecount when frames are corrupted at the end
    for (; m_imageCount > 0; m_imageCount--) {
        cv::Mat img;
        ReaderResult rres = getImage(m_imageCount - 1, img);
        if (!img.empty()) break;  // found functional frame
    }

    return ReaderResult::Success;
}

VideoReader::~VideoReader() {
    // 1) free buffered frames
    for (auto& kv : m_buffer) {
        if (kv.second) {
            av_frame_free(&kv.second);
        }
    }
    m_buffer.clear();

    // 2) free scaler
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    // 3) free decoder
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);  // sets to nullptr
    }

    // 4) CLOSE input (this releases the Windows file lock)
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);  // sets to nullptr
    }

    // DO NOT call avformat_free_context() after avformat_close_input().
}

int VideoReader::openFormatContext(const std::string& path) {
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) return -1;
    int input_res =
        avformat_open_input(&m_formatContext, path.c_str(), NULL, NULL);
    if (input_res < 0) return input_res;
    printf("Format %s, duration %ld us\n", m_formatContext->iformat->long_name,
           m_formatContext->duration);
    int find_res = avformat_find_stream_info(m_formatContext, NULL);
    if (find_res < 0) return find_res;

    return 0;
}

int VideoReader::selectVideoStream() {
    for (m_streamId = 0; m_streamId < m_formatContext->nb_streams;
         m_streamId++) {
        AVStream* stream = m_formatContext->streams[m_streamId];
        if (!stream) continue;
        AVCodecParameters* codecParams = stream->codecpar;
        if (!codecParams) continue;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) continue;
        AVMediaType codecType = codecParams->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO) {
            m_imageCount = stream->nb_frames;
            if (m_imageCount == 0) return -1;
            m_rational_avgVideoFPS = stream->avg_frame_rate;
            m_startTimestamp = stream->start_time;
            m_streamTimeBase = stream->time_base;
            return 0;
        }
    }
    return -1;
}

int VideoReader::openCodec() {
    AVCodecParameters* codecParams =
        m_formatContext->streams[m_streamId]->codecpar;
    if (!codecParams) return -1;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) return -1;
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) return -1;
    printf("Codec: %s\n", codec->name);
    int param_res = avcodec_parameters_to_context(m_codecContext, codecParams);
    if (param_res < 0) return param_res;
    AVDictionary* notFoundOptions = nullptr;
    int open_res = avcodec_open2(m_codecContext, codec, &notFoundOptions);
    if (open_res < 0) return open_res;

    return 0;
}

int VideoReader::updateSWSContext(const int width, const int height,
                                  AVPixelFormat pixFormat) {
    m_swsContext = sws_getCachedContext(
        m_swsContext, width, height, pixFormat, width, height,
        AVPixelFormat::AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

    return !m_swsContext;
}

std::pair<ReaderResult, cv::Mat> VideoReader::retrieveImage(uint index) {
    if (index >= m_imageCount) return {ReaderResult::OutOfBound, cv::Mat()};

    std::map<uint, AVFrame*>::iterator iter = m_buffer.find(index);
    const bool backwardsSeek = (int)index < m_lastFrameIdx;
    const bool inBuffer = iter != m_buffer.end();
    const bool longRangeSeek =
        abs((int)(m_lastFrameIdx - index)) >
        m_rational_avgVideoFPS.num / m_rational_avgVideoFPS.den;
    if (!inBuffer && (backwardsSeek || longRangeSeek)) {
        // jump to last I-Frame before index
        int64_t timeStampInStreamTime =
            m_startTimestamp +
            av_rescale_q(index,
                         AVRational{m_rational_avgVideoFPS.den,
                                    m_rational_avgVideoFPS.num},
                         m_streamTimeBase);
        int seek_res =
            av_seek_frame(m_formatContext, m_streamId, timeStampInStreamTime,
                          AVSEEK_FLAG_BACKWARD);
        if (seek_res < 0) return {ReaderResult::CorruptedFrame, cv::Mat()};
    }

    // sequential read until index is reached
    uint seqReadFrames = 0;
    while (iter == m_buffer.end()) {
        if (m_buffer.size() >
            m_rational_avgVideoFPS.num / m_rational_avgVideoFPS.den) {
            for (auto& d_iter : m_buffer) {
                if (d_iter.second) av_frame_free(&d_iter.second);
            }
            m_buffer.clear();
        }

        std::vector<int> seqDecodedIdx = {};
        int decode_res = decodeNextPkg(seqDecodedIdx);
        switch (decode_res) {
            case 0:  // more frames to decode
                break;
            case AVERROR(EAGAIN):  // pkg fully decoded
                continue;
            case AVERROR_EOF:  // reset cause we reached end of file
                avcodec_flush_buffers(m_codecContext);
                break;
            default:  // res < 0 (some error)
                return {ReaderResult::CorruptedFrame, cv::Mat()};
        }

        if (decode_res == AVERROR_EOF && iter == m_buffer.end()) {
            return {ReaderResult::CorruptedFrame, cv::Mat()};
        }

        iter = m_buffer.find(index);

        bool longSeqDecode = ++seqReadFrames > 2 * m_rational_avgVideoFPS.num /
                                                   m_rational_avgVideoFPS.den;
        bool lastSeqIdxHigher =
            seqDecodedIdx.size() > 0 && *(seqDecodedIdx.end() - 1) > index;
        if (longSeqDecode && lastSeqIdxHigher) {
            // if frame was not found in 2s of sequentially read frames and the
            // index last decoded index is lower anyways, its unlikely that the
            // frame will be found
            return {ReaderResult::FrameSearchTimeout, cv::Mat()};
        }
    }
    cv::Mat img = avFrame2CvMat(iter->second);
    if (img.empty()) return {ReaderResult::CorruptedFrame, cv::Mat()};

    return {ReaderResult::Success, img};
}

int VideoReader::decodeNextPkg(std::vector<int>& decodedIdx) {
    AVPacket* packet = av_packet_alloc();
    int read_res = av_read_frame(m_formatContext, packet);
    if (read_res == AVERROR_EOF) {
        packet = nullptr;  // send flush packet
    } else if (read_res < 0) {
        av_packet_free(&packet);
        return read_res;
    }

    // ignore streams that are not video
    if (packet && packet->stream_index != m_streamId) {
        av_packet_free(&packet);
        return 0;
    }

    int send_res = avcodec_send_packet(m_codecContext, packet);
    av_packet_free(&packet);

    int receive_res = 0;
    while (receive_res == 0) {
        AVFrame* av_frame = nullptr;
        av_frame = av_frame_alloc();
        receive_res = avcodec_receive_frame(m_codecContext, av_frame);
        if (receive_res == AVERROR(EAGAIN)) {
            av_frame_free(&av_frame);
            break;  // pkg fully decoded
        } else if (receive_res < 0) {
            av_frame_free(&av_frame);
            return receive_res;
        }
        int64_t idx = av_rescale_q(
            av_frame->pts - m_startTimestamp, m_streamTimeBase,
            AVRational{m_rational_avgVideoFPS.den, m_rational_avgVideoFPS.num});
        m_buffer[idx] = av_frame;
        decodedIdx.push_back(idx);
        m_lastFrameIdx = idx;
    }

    return 0;
}

bool VideoReader::calcAvgFps() {
    if (m_rational_avgVideoFPS.num <= 0 && m_rational_avgVideoFPS.den <= 0)
        return false;
    ReaderImpl::m_avgFps = av_q2d(m_rational_avgVideoFPS);
    return true;
}

bool VideoReader::calcDuration() {
    if (!m_formatContext->duration) return false;
    int64_t d_streamTime = m_formatContext->duration;
    if (d_streamTime <= 0) return false;

    int64_t d_sec =
        av_rescale_q(d_streamTime, AV_TIME_BASE_Q, AVRational({1, 1}));
    ReaderImpl::m_duration = (double)d_sec;
    return true;
}

cv::Mat VideoReader::avFrame2CvMat(const AVFrame* av_f) {
    const int h = av_f->height;
    const int w = av_f->width;
    if (av_f->format < 0) return cv::Mat();

    const AVPixelFormat pixFormat = static_cast<AVPixelFormat>(av_f->format);

    if (updateSWSContext(w, h, pixFormat) < 0) return cv::Mat();

    cv::Mat cv_f(h, w, CV_8UC3);
    // uint8_t* cv_data[] = {cv_f.data};
    // int cv_lineSize[] = {static_cast<int>(cv_f.step[0])};
    int cv_linesizes[1] = {(int)cv_f.step1()};
    const int out_h = sws_scale(m_swsContext, av_f->data, av_f->linesize, 0, h,
                                &cv_f.data, cv_linesizes);

    if (out_h != h) return cv::Mat();
    return cv_f;
}
