#include "backupvideoreader.h"

#include <QFileInfo>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

BackupVideoReader::BackupVideoReader(const QString& path,
                                     std::shared_ptr<ReaderParams> readerParams)
    : m_path(path.toUtf8().constData()), m_readerParams(readerParams) {
    QFileInfo info(path);
    if (!info.isFile()) {
        m_isValid = false;
        return;
    }
    cv::VideoCapture prev(m_path, cv::CAP_FFMPEG);
    m_numImages = prev.get(cv::CAP_PROP_FRAME_COUNT) - 1;
    m_fps = prev.get(cv::CAP_PROP_FPS);
    m_cap = prev;
    if (m_numImages > 0) {
        m_isValid = true;
    } else {
        m_isValid = false;
    }
}

BackupVideoReader::~BackupVideoReader() { m_cap.release(); }

void BackupVideoReader::addMetaData(MetaData* md) { m_md = md; }

MetaData* BackupVideoReader::getMetaData() { return m_md; }

bool BackupVideoReader::isValid() { return m_isValid; }

cv::Mat BackupVideoReader::getPic(unsigned int index,
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

unsigned int BackupVideoReader::getPicCount() { return m_numImages; }

QString BackupVideoReader::getInputPath() {
    return QString::fromStdString(m_path);
}

double BackupVideoReader::getFPS() { return m_fps; }

double BackupVideoReader::getVideoDuration() {
    return (double)m_numImages / m_fps;
}

bool BackupVideoReader::isDir() { return false; }

BackupVideoReader* BackupVideoReader::copy(
    std::shared_ptr<ReaderParams> params) {
    std::shared_ptr<ReaderParams> p = params;
    if (!p) {  // if no params are provided, copy the old ones
        p = std::make_shared<ReaderParams>(*m_readerParams);
    }  // copy cv::VideoCapture crashes, so create new instead of copy
    BackupVideoReader* reader =
        new BackupVideoReader(QString::fromStdString(m_path), p);
    reader->addMetaData(m_md);
    return reader;
}

std::vector<std::string> BackupVideoReader::getFileVector() {
    return std::vector<std::string>();
}

SequentialReader* BackupVideoReader::createSequentialReader(
    std::vector<uint> indices, PictureProcessingFlags flags) {
    return new SequentialReaderImpl(this, indices, true, flags);
}
