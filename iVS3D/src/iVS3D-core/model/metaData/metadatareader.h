#ifndef METADATAREADER_H
#define METADATAREADER_H

#include <QObject>

/**
 * @interface MetaDataReader
 *
 * @ingroup Model
 *
 * @brief Interface to load meta data
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class MetaDataReader
{
public:
    /**
     * @brief ~MetaDataReader Destructor
     */
    virtual ~MetaDataReader() = default;
    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    virtual QString getName() = 0;
    /**
     * @brief getImageMetaData Returns parsed meta data from the index images
     * @param index Index of the image to get the meat data from
     * @return QVaraint containing the meta data
     */
    virtual QVariant getImageMetaData(uint index) = 0;
    /**
     * @brief getAllMetaData Returns all meta data in the same order as the images ar
     * @return QList with the meta data from all images
     */
    virtual QList<QVariant> getAllMetaData() = 0;
    /**
     * @brief parseDataVideo Tries to load meta data for a video from the given file
     * @param path Path to the meta data file
     * @param picCount amount of images in the video
     * @param fps fps of the video
     * @param interpolate indicades wether the reader is allowed to interpolate missing meta data
     * @return @a True if meta data have been loaded @a False otherwise
     */
    virtual bool parseDataVideo(QString path, int picCount, double fps, bool interpolate) { (void)path; (void)picCount; (void)fps; (void)interpolate; return false;};

    /**
     * @brief parseDataImage Tries to load meta data from the imported images (based on exif)
     * @param paths vector containing all image paths
     * @param interpolate indicades if the reader is allowed to interpolate missing meta data
     * @return @a True if meta data have been loaded @a False otherwise
     */
    virtual bool parseDataImage(std::vector<std::string> paths, bool interpolate) { (void)paths; (void)interpolate; return false;};


};

#endif // METADATAREADER_H
