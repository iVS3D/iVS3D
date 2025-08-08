#include "videoreader.h"

VideoReader::VideoReader(const QString &path,
                         std::shared_ptr<ReaderParams> readerParams)
    : m_path(path.toUtf8().constData()), m_readerParams(readerParams) {
    QFileInfo info(path);
    if (!info.isFile()) {
        m_isValid = false;
        return;
    }

    m_formatContext = avformat_alloc_context();
    assert(m_formatContext);
        avformat_open_input(&m_formatContext, m_path.c_str(), NULL, NULL);
    printf("Format %s, duration %ld us\n", m_formatContext->iformat->long_name,
           m_formatContext->duration);
    avformat_find_stream_info(m_formatContext, NULL);

    // Select Video Stream
    const AVCodec *codec = NULL;
    AVCodecParameters *codecParams = nullptr;
    for (m_streamId = 0; m_streamId < m_formatContext->nb_streams;
         m_streamId++) {
        AVStream *stream = m_formatContext->streams[m_streamId];
        codecParams = stream->codecpar;
        codec = avcodec_find_decoder(codecParams->codec_id);
        assert(codec);
        AVMediaType codecType = codecParams->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO) {
            m_frameCount = stream->nb_frames;
            m_avgVideoFPS = stream->avg_frame_rate;
            m_startTimestamp = stream->start_time;
            m_streamTimeBase = stream->time_base;
            break;
        }
    }

    // Open CODEC
    m_codecContext = avcodec_alloc_context3(codec);
    assert(m_codecContext);
    printf("Codec: %s\n", codec->name);
    avcodec_parameters_to_context(m_codecContext, codecParams);
    AVDictionary *notFoundOptions = nullptr;
    avcodec_open2(m_codecContext, codec, &notFoundOptions);
    // TODO: display not found options

    // Create SwsContext for conversion
    AVPixelFormat pixFormat = m_codecContext->pix_fmt;
    // replaceDeprecatedPixFmt(pixFormat);
    int m_outW = m_codecContext->width;
    int m_outH = m_codecContext->height;
    m_swsContext =
        sws_getContext(m_codecContext->width, m_codecContext->height, pixFormat,
                       m_outW, m_outH, AVPixelFormat::AV_PIX_FMT_BGR24,
                       SWS_BILINEAR, NULL, NULL, NULL);
    assert(m_swsContext);

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
}

VideoReader::~VideoReader() {
    sws_freeContext(m_swsContext);
    avformat_free_context(m_formatContext);
}

void VideoReader::addMetaData(MetaData *md) { m_md = md; }

MetaData *VideoReader::getMetaData() { return m_md; }

bool VideoReader::isValid() { return m_isValid; }

cv::Mat VideoReader::getPic(unsigned int index, PictureProcessingFlags flags) {
    QMutexLocker locker(&m_mutex);

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
    assert(index < m_frameCount);
    AVPacket *packet = nullptr;
    packet = av_packet_alloc();
    while (iter == m_buffer.end()) {
        if (m_buffer.size() > m_avgVideoFPS.num / m_avgVideoFPS.den) {
            for (auto d_iter : m_buffer) {
                AVFrame *av_frame = d_iter.second;
                av_frame_free(&av_frame);
                d_iter.second = nullptr;
            }
            m_buffer.clear();
        }

        av_read_frame(m_formatContext, packet);
        int send_res = avcodec_send_packet(m_codecContext, packet);
        if (send_res == AVERROR(EINVAL) && m_codecContext->codec) {
            avcodec_send_packet(m_codecContext, NULL);  // send flush packet
        }
        int receive_res = 0;
        while (receive_res == 0) {
            AVFrame *av_frame = nullptr;
            av_frame = av_frame_alloc();
            receive_res = avcodec_receive_frame(m_codecContext, av_frame);
            if (receive_res != 0) break;
            int64_t idx =
                av_rescale_q(av_frame->pts, m_streamTimeBase,
                             AVRational{m_avgVideoFPS.den, m_avgVideoFPS.num});
            m_buffer[idx] = av_frame;
            m_lastFrameIdx = idx;
        }
        iter = m_buffer.find(index);
    }
    av_packet_free(&packet);
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

unsigned int VideoReader::getPicCount() { return m_frameCount; }

QString VideoReader::getInputPath() { return QString::fromStdString(m_path); }

double VideoReader::getFPS() { return av_q2d(m_avgVideoFPS); }

double VideoReader::getVideoDuration() {
    int64_t d_streamTime = m_formatContext->duration;
    int64_t d_sec =
        av_rescale_q(d_streamTime, m_streamTimeBase, AVRational({1, 1}));
    return d_sec;
}

bool VideoReader::isDir() { return false; }

VideoReader *VideoReader::copy() {
    return new VideoReader(QString::fromStdString(m_path), m_readerParams);
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
