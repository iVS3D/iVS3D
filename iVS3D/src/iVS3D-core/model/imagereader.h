#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include "reader.h"
#include <QObject>
#include <opencv2/core.hpp>


#include <opencv2/core/utils/filesystem.hpp>
#include <opencv2/imgcodecs.hpp>

#include <QDir>
#include <QFileInfo>
#include <QCollator>




/**
 * @class ImageReader
 *
 * @ingroup Model
 *
 * @brief The ImageReader class provides input metadata aswell as on demand image reading/loading
 *
 * @author Daniel Brommer
 *
 * @date 2021/04/14
 */
class ImageReader : public Reader

{
public:
    /**
     * @brief ImageReader constructor that will set up member variables to provide on demand reading images and read the input metadata
     * @param path directory/path of input images
     */
    explicit ImageReader(QString path);
    /**
     * @brief Returns the frame to a given index
     *
     * @param index Index of the frame to be returned
     * @param useMultipleAccess optinal paramter, if multipleAccess should be used (set to false by default)
     * @return cv::Mat of the selected frame
     */
    cv::Mat getPic(unsigned int index, bool = false) override;
    /**
     * @brief getPicCount returns count of images in input
     * @return returns image count
     */
    unsigned int getPicCount() override;
    /**
     * @brief getFPS method from interface Reader, that doesn't make sense for this class
     * @return code "-1" to signal, that an image folder doesn't have fps
     */
    double getFPS() override;
    /**
     * @brief getVideoDurationmethod from interface Reader, that doesn't make sense for this class
     * @return code "-1" to signal, that an image folder doesn't have video duration
     */
    double getVideoDuration() override;
    /**
     * @brief getInputPath getter for initially given directory
     * @return path from which the reader reads images
     */
    QString getInputPath() override;
    /**
     * @brief isDir returns true, because the ImageReader works on a directory
     * @return @a true
     */
    bool isDir() override;
    /**
     * @brief getFileVector getter for entire image path list
     * @return list of all image paths, a single entry is one full image path
     */
    std::vector<std::string> getFileVector() override;
    /**
     * @brief copy creates a new instance of ImageReader that is a copy of itself
     * @return ImageReader that is an exact copy
     */
    ImageReader *copy() override;

private:
    ImageReader();
    std::vector<std::string> m_filePaths;
    unsigned int m_numImages = 0;
    std::string m_folderPath;
};

#endif // IMAGEREADER_H
