#ifndef VIDEOREADER_H
#define VIDEOREADER_H

#include "readerfactory.h"
#include "reader.h"
#include <QObject>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QMutex>
#include <QMutexLocker>
#include "algorithmmanager.h"
#include "sequentialreaderimpl.h"


/**
 * @class VideoReader
 *
 * @ingroup Model
 *
 * @brief The VideoReader class is used to import video files. Implements the Reader interface
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/05
 */

class VideoReader : public Reader

{
public:
    /**
     * @brief VideoReader constructor which reads the given file and creates cv::VideoCapture from it.
     *
     * @param path Path to the video. Video can be the types, which cv::VideoCapture can handle
     */
    explicit VideoReader(const QString &path);
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
    cv::Mat getPic(unsigned int index) override;
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
    VideoReader *copy() override;

    /**
     * @brief Returns a empty vector
     *
     * @return empty vector
     */
    std::vector<std::string> getFileVector() override;

    /**
     * @brief createSequentialReader Creates a SequentialReader object for the given indices.
     * This can be used to access images concurrently if the images are known a priori.
     * @param indices The indices of the images that will be accessed
     * @return SequentialReader instance (caller takes ownership!)
     */
    SequentialReader *createSequentialReader(std::vector<uint> indices) override;

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
    int m_currentIndex = 0;
    std::string m_path;
    unsigned int m_numImages;
    cv::VideoCapture m_cap;
    bool m_isValid = false;
    double m_fps;
    MetaData* m_md = nullptr;
};

REGISTER_READER("VideoReader", VideoReader)

#endif // VIDEOREADER_H
