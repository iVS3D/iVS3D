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

void VideoReader::initMultipleAccess(const std::vector<uint> &frames) {
    if (frames.size() == 0) {
        return;
    }
    m_currentFrames = frames;
    m_multipleAccess = true;
    m_currentMultipleIndex = 0;
    int firstIndex = m_currentFrames[0];
    if (firstIndex == 0) {
        m_cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        m_lastUsedIndex = -1;
        return;
    }
    m_cap.set(cv::CAP_PROP_POS_FRAMES, firstIndex - 1);
    m_lastUsedIndex = firstIndex;

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


cv::Mat VideoReader::getPic(unsigned int index, bool useMultipleAccess)
{

    QMutexLocker locker(&mutex);
    //Prevent invalid request
    if(index >= getPicCount()){
        cv::Mat empty;
        return empty;
    }
    cv::Mat ret;
    //When Multiple Acess is active wait for the next request in the keyframes and then use sequentiell acess
    if (m_multipleAccess  && useMultipleAccess) {
        //Check if index is the next frame in the given vector
        if (index == m_currentFrames[m_currentMultipleIndex]) {
            while(m_lastUsedIndex < (int)index) {
                cv::Mat i;
                m_cap.read(i);
                m_lastUsedIndex++;
            }
            m_cap.read(ret);
            m_lastUsedIndex++;
            m_currentMultipleIndex++;
            //Check if current access was last one
            if (index == m_currentFrames.back()) {
                m_multipleAccess = false;
                m_currentMultipleIndex = 0;
            }
            return ret;
        }
        //Return emtpty mat if current request isnt the next in line
        else {
            cv::Mat empty;
            return empty;
        }

    }
    // switches between random access and consecutive access if posible
    if (m_lastUsedIndex + 1 == index) {
        m_cap.read(ret);
    } else {
        m_cap.set(cv::CAP_PROP_POS_FRAMES, index);
        m_cap.read(ret);
    }
    m_lastUsedIndex = index;
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
    if (m_multipleAccess) {
        reader->initMultipleAccess(m_currentFrames);
    }
    reader->addMetaData(m_md);
    return reader;
}

std::vector<std::string> VideoReader::getFileVector()
{
   return std::vector<std::string>();
}
