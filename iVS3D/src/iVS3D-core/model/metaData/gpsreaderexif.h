#ifndef GPSREADEREXIF_H
#define GPSREADEREXIF_H

#include "gpsreader.h"
#include "metadatamanager.h"
#include "exif.h"
#include <QVariant>
#include <QPointF>

class GPSReaderExif : public GPSReader
{
public:
    GPSReaderExif();
    ~GPSReaderExif();

    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    QString getName();
    /**
     * @brief getImageMetaData Returns parsed meta data from the index images
     * @param index Index of the image to get the meat data from
     * @return QVaraint containing the meta data
     */
    QVariant getImageMetaData(uint index);
    /**
     * @brief getAllMetaData Returns all meta data in the same order as the images ar
     * @return QList with the meta data from all images
     */
    QList<QVariant> getAllMetaData();
    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseDataImage(std::vector<std::string> paths);

private:
    QString m_name = "GPSReaderExif";
    QString m_path;
    QList<QPointF> m_GPSvalues;
};

REGISTER_METAREADER("GPSReaderExif", GPSReaderExif)

#endif // GPSREADEREXIF_H
