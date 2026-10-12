#include "fastvideoreader.h"

#include <QFileInfo>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

FastVideoReader::FastVideoReader(const QString& path,
                                 std::shared_ptr<ReaderParams> readerParams)
    : m_path(path.toUtf8().constData()), m_readerParams(readerParams) {
    QFileInfo info(path);
    if (!info.isFile()) {
        m_isValid = false;
        return;
    }
    m_cap = cv::VideoCapture(m_path, cv::CAP_FFMPEG);
    m_numImages = m_cap.get(cv::CAP_PROP_FRAME_COUNT) - 1;
    m_fps = m_cap.get(cv::CAP_PROP_FPS);
    if (m_numImages > 0 && m_fps >= 1) {
        m_isValid = true;
    } else {
        m_isValid = false;
        m_cap.release();
    }
}

FastVideoReader::~FastVideoReader() { m_cap.release(); }

void FastVideoReader::addMetaData(MetaData* md) { m_md = md; }

MetaData* FastVideoReader::getMetaData() { return m_md; }

bool FastVideoReader::isValid() { return m_isValid; }

cv::Mat FastVideoReader::getPic(unsigned int index,
                                PictureProcessingFlags flags) {
    QMutexLocker locker(&m_mutex);

    // minimum distance from the current index in the video to the next index
    // for jumping. If the distance to the next index is less, reading frame by
    // frame is faster else jumping there might be faster (depending on the
    // video length, resolution, etc.) This value was chosen empirically on a
    // modern desktop PC with Windows 11 (May 2023)
    const int MIN_JUMP_DISTANCE = 40;

    // invalid index requested
    if (index >= getPicCount()) {
        cv::Mat empty;
        return empty;
    }

    cv::Mat ret;

    // Jump if:
    // - going backwards
    // - going forward more than MIN_JUMP_DISTANCE
    if (index <= (uint)m_currentIndex ||
        index >= (uint)(m_currentIndex + MIN_JUMP_DISTANCE)) {
        // jump to the desired index
        m_cap.set(cv::CAP_PROP_POS_FRAMES, index);
        m_cap.read(ret);
        m_currentIndex = index;
    } else {
        // grab images sequentially until index is reached
        while (m_currentIndex < (int)index - 1) {
            m_cap.grab();
            m_currentIndex++;
        }
        m_cap.read(ret);
        m_currentIndex++;
    }

    // apply processing
    if (flags & PictureProcessingFlags::APPLY_RESIZING) {
        m_readerParams->getWorkingResolution().resize(ret);
    }
    if (flags & PictureProcessingFlags::APPLY_CROPPING &&
        m_readerParams->getUseRoi()) {
        m_readerParams->getRoi().crop(ret);
    }
    return ret;
}

unsigned int FastVideoReader::getPicCount() { return m_numImages; }

QString FastVideoReader::getInputPath() {
    return QString::fromStdString(m_path);
}

double FastVideoReader::getFPS() { return m_fps; }

double FastVideoReader::getVideoDuration() {
    return (double)m_numImages / m_fps;
}

bool FastVideoReader::isDir() { return false; }

FastVideoReader* FastVideoReader::copy(std::shared_ptr<ReaderParams> params) {
    std::shared_ptr<ReaderParams> p = params;
    if (!p) {  // if no params are provided, copy the old ones
        p = std::make_shared<ReaderParams>(*m_readerParams);
    }  // copy cv::VideoCapture crashes, so create new instead of copy
    FastVideoReader* reader =
        new FastVideoReader(QString::fromStdString(m_path), p);
    reader->addMetaData(m_md);
    return reader;
}

std::vector<std::string> FastVideoReader::getFileVector() {
    return std::vector<std::string>();
}

SequentialReader* FastVideoReader::createSequentialReader(
    std::vector<uint> indices, PictureProcessingFlags flags) {
    return new SequentialReaderImpl(this, indices, true, flags);
}
