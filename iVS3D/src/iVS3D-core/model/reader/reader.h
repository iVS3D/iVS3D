#ifndef READER_H
#define READER_H

#include <QObject>
#include <opencv2/core.hpp>
#include "metadata.h"
#include "sequentialreader.h"

/**
 * @interface Reader
 *
 * @ingroup Model
 *
 * @brief The Reader interface defines functions which are used for reading and parsing the import
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/05
 */

class Reader
{
public:
    virtual ~Reader() {};
    /**
     * @brief Returns the frame to a given index
     *
     * @param index Index of the frame to be returned
     * @param useMultipleAccess optinal paramter, if multipleAccess should be used (set to false by default)
     * @return cv::Mat of the selected frame
     */
    virtual cv::Mat getPic(unsigned int index) = 0;
    /**
     * @brief Returns the number of frame
     *
     * @return Number of frame
     */
    virtual unsigned int getPicCount() = 0;
    /**
     * @brief Returns the input path
     *
     * @return Qstring with the input path
     */
    virtual QString getInputPath() = 0;
    /**
     * @brief Returns the video FPS
     *
     * @return double with the FPS, -1 if input isn't a video
     */
    virtual double getFPS() = 0;
    /**
     * @brief Returns the video duration
     *
     * @return double with the video duration, -1 if input isn't a video
     */
    virtual double getVideoDuration() = 0;
    /**
     * @brief Returns wether the input is a direcory or not
     *
     * @return @a true if the input is based on a directory, @a false otherwise
     */
    virtual bool isDir() = 0;
    /**
     * @brief Creates this reader again and returns it
     *
     * @return New instance of this reader
     */
    virtual Reader *copy() = 0;
    /**
     * @brief Returns a vector with filepaths (only valid, if the reader is a imagereader)
     *
     * @return Vector with filepaths
     */
    virtual std::vector<std::string> getFileVector() = 0;

    virtual SequentialReader *createSequentialReader(std::vector<uint> indices) = 0;

    /**
     * @brief enableMultithreading This method has to be called once in the plugins to use the reader while multithreading
     */
    virtual void enableMultithreading() {}
    /**
     * @brief addMetaData Used to add MetaData to the reader
     * @param md The MetaData to be saved
     */
    virtual void addMetaData(MetaData* md) = 0;
    /**
     * @brief getMetaData Returns the currently saved MetaData
     * @return The currently saved MetaData
     */
    virtual MetaData* getMetaData() = 0;
    /**
     * @brief isValid Retruns wether the reader is valid or not
     * @return @a true if the reader is valid, @a false otherwise
     */
    virtual bool isValid() = 0;

};

#endif // READER_H
