#ifndef GPSREADEREXIFPNG_H
#define GPSREADEREXIFPNG_H

#include "gpsreader.h"
#include "metadatamanager.h"
#include "exif.h"
#include <QVariant>
#include <QPointF>

class GPSReaderExifPNG: public GPSReader
{
public:
    GPSReaderExifPNG();
    ~GPSReaderExifPNG();

    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    QString getName();
    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @param interpolate indicades wether the reader is allowed to interpolate missing meta data
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseDataImage(std::vector<std::string> paths, bool interpolate);

private:
    QString m_name = "GPSReaderExifPNG";
};

REGISTER_METAREADER("GPSReaderExifPNG", GPSReaderExifPNG)

#endif // GPSREADEREXIFPNG_H
