#include "videoreader.h"

VideoReader::VideoReader(const QString &path,
                         std::shared_ptr<ReaderParams> readerParams)
    : m_path(path.toUtf8().constData()), m_readerParams(readerParams) {
    QFileInfo info(path);
    if (!info.isFile()) {
        m_isValid = false;
        return;
    }

    openFormatContext();
    selectVideoStream();
    openCodec();
    createSWS();

    // Create SwsContext for conversion
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
    m_isValid = true;

    // reduce framecount when frames are corrupted at the end
    for (uint idx = m_frameCount-1; idx > 0; --idx) {
        cv::Mat img = getPic(idx, APPLY_NONE);
        if (!img.empty()) break; // found functional frame
        m_frameCount--;
    }
}

VideoReader::~VideoReader() {
    sws_freeContext(m_swsContext);
    avformat_free_context(m_formatContext);
}

void VideoReader::openFormatContext() {
    m_formatContext = avformat_alloc_context();
    assert(m_formatContext);
    avformat_open_input(&m_formatContext, m_path.c_str(), NULL, NULL);
    printf("Format %s, duration %ld us\n", m_formatContext->iformat->long_name,
           m_formatContext->duration);
    avformat_find_stream_info(m_formatContext, NULL);
}

void VideoReader::selectVideoStream() {
    for (m_streamId = 0; m_streamId < m_formatContext->nb_streams;
         m_streamId++) {
        AVStream *stream = m_formatContext->streams[m_streamId];
        AVCodecParameters *codecParams = stream->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
        AVMediaType codecType = codecParams->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO) {
            m_frameCount = stream->nb_frames;
            m_avgVideoFPS = stream->avg_frame_rate;
            m_startTimestamp = stream->start_time;
            m_streamTimeBase = stream->time_base;
            break;
        }
    }
}

void VideoReader::openCodec() {
    AVCodecParameters *codecParams =
        m_formatContext->streams[m_streamId]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    m_codecContext = avcodec_alloc_context3(codec);
    assert(m_codecContext);
    printf("Codec: %s\n", codec->name);
    avcodec_parameters_to_context(m_codecContext, codecParams);
    AVDictionary *notFoundOptions = nullptr;
    avcodec_open2(m_codecContext, codec, &notFoundOptions);
    // TODO: display not found options
}

void VideoReader::createSWS() {
    AVPixelFormat pixFormat = m_codecContext->pix_fmt;
    // replaceDeprecatedPixFmt(pixFormat);
    int m_outW = m_codecContext->width;
    int m_outH = m_codecContext->height;
    m_swsContext =
        sws_getContext(m_codecContext->width, m_codecContext->height, pixFormat,
                       m_outW, m_outH, AVPixelFormat::AV_PIX_FMT_BGR24,
                       SWS_BILINEAR, NULL, NULL, NULL);
    assert(m_swsContext);
}

void VideoReader::addMetaData(MetaData *md) { m_md = md; }

MetaData *VideoReader::getMetaData() { return m_md; }

bool VideoReader::isValid() { return m_isValid; }

cv::Mat VideoReader::getPic(unsigned int index, PictureProcessingFlags flags) {
    QMutexLocker locker(&m_mutex);

    if (index >= m_frameCount)
        return cv::Mat();

    std::map<uint, AVFrame *>::iterator iter = m_buffer.find(index);
    const bool backwardsSeek = index < m_lastFrameIdx;
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
        av_seek_frame(m_formatContext, m_streamId, timeStampInStreamTime,
                      AVSEEK_FLAG_BACKWARD);
    }

    // sequential read until index is reached
    while (iter == m_buffer.end()) {
        if (m_buffer.size() > m_avgVideoFPS.num / m_avgVideoFPS.den) {
            for (auto &d_iter : m_buffer) {
                av_frame_free(&d_iter.second);
            }
            m_buffer.clear();
        }

        int decode_res = decodeNextPkg();
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

        iter = m_buffer.find(index);
        if (decode_res == AVERROR_EOF && iter == m_buffer.end()) {
            return cv::Mat();
        }
    }

    // ------------------- DEBUG -------------------
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(8) << index << ".y";
    const std::string fullPath = "/home/dom26496/Dokumente/TestDaten/tst/" + ss.str();

    FILE* f;
    f = fopen(fullPath.c_str(), "w");
    const int w = iter->second->width;
    const int h = iter->second->height;
    for (int y = 0; y < h; y++) {
        fwrite(iter->second->data[0] + y * iter->second->linesize[0], 1, w, f);
    }
    fclose(f);
    // ---------------------------------------------

    cv::Mat img = avFrame2CvMat(iter->second);

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

int VideoReader::decodeNextPkg() {
    AVPacket *packet = av_packet_alloc();
    int read_res = av_read_frame(m_formatContext, packet);
    if (read_res == AVERROR_EOF) {
        av_packet_free(&packet);
        packet = NULL;  // send flush packet
    } else if (read_res < 0) {
        av_packet_free(&packet);
        return read_res;
    }
    avcodec_send_packet(m_codecContext, packet);

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
            av_packet_free(&packet);
            return receive_res;
        }
        int64_t idx =
            av_rescale_q(av_frame->pts - m_startTimestamp, m_streamTimeBase,
                         AVRational{m_avgVideoFPS.den, m_avgVideoFPS.num});
        m_buffer[idx] = av_frame;
        m_lastFrameIdx = idx;
    }

    av_packet_free(&packet);
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
    const Resolution res = m_readerParams->getOriginalResolution();
    const uint w = res.getWidth();
    const uint h = res.getHeight();

    cv::Mat cv_f(h, w, CV_8UC3);
    const int cv_lineSize = cv_f.step1();
    sws_scale(m_swsContext, av_f->data, av_f->linesize, 0,
              m_codecContext->height, &cv_f.data, &cv_lineSize);

    return cv_f;
}
