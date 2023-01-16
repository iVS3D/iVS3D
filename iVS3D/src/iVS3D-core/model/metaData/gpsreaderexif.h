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
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseDataImage(std::vector<std::string> paths);

private:
    QString m_name = "GPSReaderExif";
};

REGISTER_METAREADER("GPSReaderExif", GPSReaderExif)

#endif // GPSREADEREXIF_H
