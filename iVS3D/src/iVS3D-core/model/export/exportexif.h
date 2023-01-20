#ifndef EXPORTEXIF_H
#define EXPORTEXIF_H

#include <QObject>
#include <QVariant>
#include <QFileInfo>
#include "stringcontainer.h"
#include <cstring>

class ExportExif
{
public:
    ExportExif();
    char* saveExif(QString Path, QVariant exifData);
    int getExifSize();

private:
    //ByteOrder (2 Bytes), 42 (2 Bytes), Offset of 0th IFD (4 Bytes)
    unsigned char* TIFFHeader;
    unsigned char* PNGChunkHeader;
    unsigned char* JPEGMarker;
    //The IFD used in this standard consists of a 2-byte count (number of fields), 12-byte field Interoperability arrays, and 4-byte offset to the next IFD
    unsigned char* GPSIFD;
    //2Bytes count of tags ind GPS IFD (4 w/o altitude, 6 with)
    unsigned char* GPSTagCount;
    //Tags are always 12 Bytes: (2bytes Tag, 2bytes Type, 4bytes count, 4bytes Value -> or Offset to value if value is more then 4 bytes)
    unsigned char* LatitudeTag;
    unsigned char* LatitudeRefTag;
    unsigned char* LongitudeTag;
    unsigned char* LongitudeRefTag;
    unsigned char* AltitudeTag;
    unsigned char* AltitudeRefTag;
    //Data fields for Lat,Long,Alt. They are type rational and therefore longer then 4 bytes.
    //3 Rationales: deg,h,s each 4 bytes numerator and 4 bytes denumerator --> 24bytes total
    unsigned char* LatitudeData;
    //3 Rationales: deg,h,s each 4 bytes numerator and 4 bytes denumerator --> 24bytes total
    unsigned char* LongitudeData;
    //1 Rational: altitude in meters, 4 bytes numerator and 4 bytes denumerator --> 8bytes total
    unsigned char* AltitudeData;
    //Offset between ifd tags and data
    unsigned char* offestToGPSData;
    //CRC for png chunk, not calculated yet
    unsigned char* PNGCRC;

    bool useAltitude;
    bool usePNG;
    int exifSize;


};

#endif // EXPORTEXIF_H
