#ifndef DELAYEDCOPYREADER_H
#define DELAYEDCOPYREADER_H

#include "reader.h"

/**
 * @class DelayedCopyReader
 *
 * @ingroup Model
 *
 * @brief The DelayedCopyReader class is a proxy for a Reader and copies this Reader if Reader::getPic() is called.
 *
 * This implementation allows to perform time consuming copying of Reader objects only if they are used.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/26
 */
class DelayedCopyReader : public Reader
{
public:
    /**
     * @brief Create a DelayedCopyReader instance which performs Reader::copy() operation on given Reader on first call of Reader::getPic().
     * @param reader the Reader to copy
     */
    explicit DelayedCopyReader(Reader *reader);

    /**
     * @brief getPic calls Reader::getPic() on copy of given Reader [COPIES THE READER IF NOT DONE YET].
     * @param index Index of the frame to be returned
     * @param useMultipleAccess optinal paramter, if multipleAccess should be used (set to false by default)
     * @return cv::Mat of the selected frame
     */
    cv::Mat getPic(unsigned int idx, bool = false) override;

    /**
     * @brief getPicCount Returns the number of pictures to access on this Reader [DOES NOT COPY THE READER].
     * @return the number of pictures
     */
    unsigned int getPicCount() override;

    /**
     * @brief getInputPath Returns the input path of the pictures [DOES NOT COPY THE READER].
     * @return the input path
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
     * @brief isDir Checks if the given Reader operates on a video or directory [DOES NOT COPY THE READER].
     * @return @a true if reading from directory
     */
    bool isDir() override;

    /**
     * @brief copy Create a copy of this DelayedCopyReader.
     * @return the copy
     */
    Reader *copy() override;
    /**
     * @brief Returns a vector with filepaths (only valid, if the reader is a imagereader)
     *
     * @return Vector with filepaths
     */
    std::vector<std::string> getFileVector() override;
    /**
     * @brief enableMultithreading This method has to be called once in the plugins to use the reader while multithreading
     */

    void enableMultithreading() override;
    /**
     * @brief initMultipleAccess Enables the reader to make ordered access to the input, which will result in a faster access time
     * @param frames Vector containing the indices of the images which will be used in ascending order
     */

    void initMultipleAccess(const std::vector<uint> &frames) override;
private:
    Reader *m_realReader;
    Reader *m_copyReader;
};

#endif // DELAYEDCOPYREADER_H
