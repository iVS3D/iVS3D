#include "videoreader.h"

VideoReader::VideoReader(const QString &path) : m_path(path.toUtf8().constData())
{
    QFileInfo info(path);
    if (!info.isFile()) {
        m_isValid = false;
        return;
    }
    cv::VideoCapture prev(m_path, cv::CAP_FFMPEG);
    m_numImages = prev.get(cv::CAP_PROP_FRAME_COUNT)-1;
    m_fps = prev.get(cv::CAP_PROP_FPS);
    m_cap = prev;
    if (m_numImages > 0) {
        m_isValid = true;
    }
    else {
        m_isValid = false;
    }
}

VideoReader::~VideoReader()
{
    m_cap.release();
}

void VideoReader::addMetaData(MetaData *md)
{
    m_md = md;
}

MetaData *VideoReader::getMetaData()
{
    return m_md;
}

bool VideoReader::isValid()
{
    return m_isValid;
}


cv::Mat VideoReader::getPic(unsigned int index)
{
    QMutexLocker locker(&m_mutex);

    // minimum distance from the current index in the video to the next index for jumping.
    // If the distance to the next index is less, reading frame by frame is faster
    // else jumping there might be faster (depending on the video length, resolution, etc.)
    // This value was chosen empirically on a modern desktop PC with Windows 11 (May 2023)
    const int MIN_JUMP_DISTANCE = 40;

    // invalid index requested
    if(index >= getPicCount()){
        cv::Mat empty;
        return empty;
    }

    cv::Mat ret;

    // Jump if:
    // - going backwards
    // - going forward more than MIN_JUMP_DISTANCE
    if(index < m_currentIndex  || index >= (m_currentIndex + MIN_JUMP_DISTANCE)){
        // jump to the desired index
        m_cap.set(cv::CAP_PROP_POS_FRAMES, index);
        m_cap.read(ret);
        m_currentIndex = index;
    } else {
        // grab images sequentially until index is reached
        while(m_currentIndex < (int)index-1) {
            m_cap.grab();
            m_currentIndex++;
        }
        m_cap.read(ret);
        m_currentIndex++;
    }
    return ret;
}

unsigned int VideoReader::getPicCount()
{
    return m_numImages;
}

QString VideoReader::getInputPath()
{
    return QString::fromStdString(m_path);
}

double VideoReader::getFPS()
{
    return m_fps;
}

double VideoReader::getVideoDuration()
{
    return (double) m_numImages / m_fps;
}

bool VideoReader::isDir()
{
    return false;
}

VideoReader *VideoReader::copy()
{
    // copy cv::VideoCapture crashes, so create new instead of copy
    VideoReader* reader =  new VideoReader(QString::fromStdString(m_path));
    reader->addMetaData(m_md);
    return reader;
}

std::vector<std::string> VideoReader::getFileVector()
{
    return std::vector<std::string>();
}

SequentialReader *VideoReader::createSequentialReader(std::vector<uint> indices)
{
    return new SequentialReaderImpl(this, indices);
}
