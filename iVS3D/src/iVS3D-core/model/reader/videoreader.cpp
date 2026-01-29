#include "videoreader.h"
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <cstdint>
#include <string>
#include <vector>

VideoReader::VideoReader(const QString &path,
                         std::shared_ptr<ReaderParams> readerParams)
    : m_path(path.toUtf8().constData()), m_readerParams(readerParams) {
    QFileInfo info(path);
    m_isValid = false;
    if (!info.isFile()) return;

    if (openFormatContext() > 0) return;
    if (selectVideoStream() > 0) return;
    if (openCodec() > 0) return;

    // validate the given readerParams, initialize if necessary!
    uint w = m_codecContext->width;
    uint h = m_codecContext->height;
    const Resolution res(w, h);
    if (!(m_readerParams->getOriginalResolution() == res)) {
        // The readerParams were initialized previously, but do not match the
        // current input resolution !
        // We just override it, but this should not happen, wrong usage?
        Q_ASSERT(!m_readerParams->getOriginalResolution().isValid());
        m_readerParams->initialize(res);
    }

    if (w < 10 || h < 10) {
        return;
    }

    m_isValid = true;

    // reduce framecount when frames are corrupted at the end
    for (uint idx = m_frameCount-1; idx > 0; --idx) {
        cv::Mat img = getPic(idx, APPLY_NONE);
        if (!img.empty()) break; // found functional frame
        m_frameCount--;
    }
}

VideoReader::~VideoReader() {

    // 1) free buffered frames
    for (auto &kv : m_buffer) {
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
        avcodec_free_context(&m_codecContext); // sets to nullptr
    }

    // 4) CLOSE input (this releases the Windows file lock)
    if (m_formatContext) {
        avformat_close_input(&m_formatContext); // sets to nullptr
    }

    // DO NOT call avformat_free_context() after avformat_close_input().
}


int VideoReader::openFormatContext() {
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) return -1;
    int inout_res = avformat_open_input(&m_formatContext, m_path.c_str(), NULL, NULL);
    if (inout_res < 0) return inout_res;
    printf("Format %s, duration %ld us\n", m_formatContext->iformat->long_name,
           m_formatContext->duration);
    int find_res = avformat_find_stream_info(m_formatContext, NULL);
    if (find_res < 0) return find_res;

    return 0;
}

int VideoReader::selectVideoStream() {
    for (m_streamId = 0; m_streamId < m_formatContext->nb_streams;
         m_streamId++) {
        AVStream *stream = m_formatContext->streams[m_streamId];
        if (!stream) continue;
        AVCodecParameters *codecParams = stream->codecpar;
        if (!codecParams) continue;
        const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) continue;
        AVMediaType codecType = codecParams->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO) {
            m_frameCount = stream->nb_frames;
            m_avgVideoFPS = stream->avg_frame_rate;
            m_startTimestamp = stream->start_time;
            m_streamTimeBase = stream->time_base;
            return 0;
        }
    }
    return -1;
}

int VideoReader::openCodec() {
    AVCodecParameters *codecParams =
        m_formatContext->streams[m_streamId]->codecpar;
    if (!codecParams) return -1;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) return -1;
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) return -1;
    printf("Codec: %s\n", codec->name);
    int param_res = avcodec_parameters_to_context(m_codecContext, codecParams);
    if (param_res < 0) return param_res;
    AVDictionary *notFoundOptions = nullptr;
    int open_res = avcodec_open2(m_codecContext, codec, &notFoundOptions);
    if (open_res < 0) return open_res;

    return 0;
}

int VideoReader::updateSWSContext(const int width, const int height,
                                  AVPixelFormat pixFormat) {
    m_swsContext = sws_getCachedContext(
        m_swsContext,
        width,
        height,
        pixFormat,
        width,
        height,
        AVPixelFormat::AV_PIX_FMT_BGR24,
        SWS_POINT,
        NULL, NULL, NULL);

    return !m_swsContext;
}

void VideoReader::addMetaData(MetaData *md) { m_md = md; }

MetaData *VideoReader::getMetaData() { return m_md; }

bool VideoReader::isValid() { return m_isValid; }

cv::Mat VideoReader::getPic(unsigned int index, PictureProcessingFlags flags) {
    QMutexLocker locker(&m_mutex);

    if (index >= m_frameCount)
        return cv::Mat();

    std::map<uint, AVFrame *>::iterator iter = m_buffer.find(index);
    const bool backwardsSeek = (int)index < m_lastFrameIdx;
    const bool inBuffer = iter != m_buffer.end();
    const bool longRangeSeek = abs((int)(m_lastFrameIdx - index)) >
                               m_avgVideoFPS.num / m_avgVideoFPS.den;
    if (!inBuffer && (backwardsSeek || longRangeSeek)) {
        // jump to last I-Frame before index
        int64_t timeStampInStreamTime =
            m_startTimestamp +
            av_rescale_q(index,
                         AVRational{m_avgVideoFPS.den, m_avgVideoFPS.num},
                         m_streamTimeBase);
        int seek_res = av_seek_frame(m_formatContext, m_streamId, timeStampInStreamTime,
                      AVSEEK_FLAG_BACKWARD);
        if (seek_res < 0)
            return cv::Mat();
    }

    // sequential read until index is reached
    uint seqReadFrames = 0;
    while (iter == m_buffer.end()) {
        if (m_buffer.size() > m_avgVideoFPS.num / m_avgVideoFPS.den) {
            for (auto &d_iter : m_buffer) {
                if (d_iter.second)
                    av_frame_free(&d_iter.second);
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
                return cv::Mat();
        }

        if (decode_res == AVERROR_EOF && iter == m_buffer.end()) {
            return cv::Mat();
        }
        iter = m_buffer.find(index);

        bool longSeqDecode =
            ++seqReadFrames > 2 * m_avgVideoFPS.num / m_avgVideoFPS.den;
        bool lastSeqIdxHigher =
            seqDecodedIdx.size() > 0 && *(seqDecodedIdx.end() - 1) > index;
        if (longSeqDecode && lastSeqIdxHigher) {
            // if frame was not found in 2s of sequentially read frames and the
            // index last decoded index is lower anyways, its unlikely that the
            // frame will be found
            return cv::Mat();
        }

    }
    cv::Mat img = avFrame2CvMat(iter->second);
    if (img.empty())
        return cv::Mat();

    // apply processing
    if (flags & PictureProcessingFlags::APPLY_RESIZING) {
        m_readerParams->getWorkingResolution().resize(img);
    }
    if (flags & PictureProcessingFlags::APPLY_CROPPING &&
        m_readerParams->getUseRoi()) {
        m_readerParams->getRoi().crop(img);
    }
    return img;
}

int VideoReader::decodeNextPkg(std::vector<int> &decodedIdx) {
    AVPacket *packet = av_packet_alloc();
    int read_res = av_read_frame(m_formatContext, packet);
    if (read_res == AVERROR_EOF) {
        packet = nullptr;  // send flush packet
    } else if (read_res < 0) {
        av_packet_free(&packet);
        return read_res;
    }
    //
    // if (packet && packet->stream_index != m_streamId) {
    //     av_packet_free(&packet);
    //     return 0;
    // }

    int send_res = avcodec_send_packet(m_codecContext, packet);
    av_packet_free(&packet);

    int receive_res = 0;
    while (receive_res == 0) {
        AVFrame *av_frame = nullptr;
        av_frame = av_frame_alloc();
        receive_res = avcodec_receive_frame(m_codecContext, av_frame);
        if (receive_res == AVERROR(EAGAIN)) {
            av_frame_free(&av_frame);
            break;  // pkg fully decoded
        } else if (receive_res < 0) {
            av_frame_free(&av_frame);
            return receive_res;
        }
        int64_t idx =
            av_rescale_q(av_frame->pts - m_startTimestamp, m_streamTimeBase,
                         AVRational{m_avgVideoFPS.den, m_avgVideoFPS.num});
        m_buffer[idx] = av_frame;
        decodedIdx.push_back(idx);
        m_lastFrameIdx = idx;
    }

    return 0;
}

unsigned int VideoReader::getPicCount() { return m_frameCount; }

QString VideoReader::getInputPath() { return QString::fromStdString(m_path); }

double VideoReader::getFPS() {
    AVRational r = m_avgVideoFPS.num ? m_avgVideoFPS : AVRational{0, 1};
    return r.num && r.den ? av_q2d(r) : 0.0;
}

double VideoReader::getVideoDuration() {
    int64_t d_streamTime = m_formatContext->duration;
    if (d_streamTime <= 0) return 0.0;
    int64_t d_sec =
        av_rescale_q(d_streamTime, AV_TIME_BASE_Q, AVRational({1, 1}));
    return (double)d_sec;
}

bool VideoReader::isDir() { return false; }

VideoReader *VideoReader::copy(std::shared_ptr<ReaderParams> params)
{
    std::shared_ptr<ReaderParams> p = params;
    if (!p) { // if no params are provided, copy the old ones
        p = std::make_shared<ReaderParams>(*m_readerParams);
    }
    // copy cv::VideoCapture crashes, so create new instead of copy
    VideoReader* reader =  new VideoReader(QString::fromStdString(m_path), p);
    reader->addMetaData(m_md);
    return reader;
}

std::vector<std::string> VideoReader::getFileVector() {
    return std::vector<std::string>();
}

SequentialReader *VideoReader::createSequentialReader(
    std::vector<uint> indices, PictureProcessingFlags flags) {
    return new SequentialReaderImpl(this, indices, true, flags);
}

cv::Mat VideoReader::avFrame2CvMat(const AVFrame *av_f) {
    const int h = av_f->height;
    const int w = av_f->width;
    if (av_f->format < 0)
        return cv::Mat();
    const AVPixelFormat pixFormat = static_cast<AVPixelFormat>(av_f->format);

    if (updateSWSContext(w, h, pixFormat) < 0)
        return cv::Mat();

    cv::Mat cv_f(h, w, CV_8UC3);
    uint8_t* cv_data[4] = {cv_f.data, nullptr, nullptr, nullptr};
    int cv_lineSize[4] = {static_cast<int>(cv_f.step[0]), 0, 0, 0};
    const int out_h = sws_scale(
        m_swsContext,
        av_f->data,
        av_f->linesize,
        0,
        h,
        cv_data,
        cv_lineSize);

    if (out_h != h)
        return cv::Mat();
    return cv_f;
}
