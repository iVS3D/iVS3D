#ifndef EXPORTEXIF_H
#define EXPORTEXIF_H

#include <QObject>
#include <QVariant>
#include <QFileInfo>
#include "stringcontainer.h"
#include <cstring>
#include <QtMath>
#include <numeric>
#include <variant>

class ExportExif
{
public:
    ExportExif();
    char* saveExif(QString Path, QVariant exifData);
    int getExifSize();

private:
    //ByteOrder (2 Bytes), 42 (2 Bytes), Offset of 0th IFD (4 Bytes)
    const unsigned char TIFFHeader[8] = { 0x49,0x49,0x2A,0x00,0x08,0x00,0x00,0x00 };
    //4Bytes length of Data Only 172-12 = 160///////////(without PNGChunkHeader and crc) --> total length - 8B PNGChunkHeader - 4B CRC
    //TODO Size does change
//    const unsigned char PNGChunkHeader[8] = { 0x00,0x00,0x00,0x00,
//                                              //eXIf Tag PNG --> eXIf
//                                              101,88,73,102 };
//    const unsigned char JPEGMarker[10] = { 0xFF,0xE1,
//                                          //Length of APP1 Field
//                                          0x00,0xAA,
//                                          //Exif
//                                          0x45,0x78,0x69,0x66,
//                                          //Offset
//                                          0x00,0x00 };
    //The IFD used in this standard consists of a 2-byte count (number of fields), 12-byte field Interoperability arrays, and 4-byte offset to the next IFD
    const unsigned char GPSIFD[18] = { //Number of fields in this IFD, 2 bytes --> only gps ifd
                                   0x01,0x00,
                                   //GPS IFD Tag, next 12Bytes is the IFD 12-byte field Interoperability array
                                   0x25,0x88,
                                   //Type -> long
                                   0x04,0x00,
                                   //Count
                                   0x01,0x00,0x00,0x00,
                                   //Value -> Pointer to gps tags 0x1A = 26: Offset from TIFF Header to GPSValues --> 8Byte TIFF + 18Bytes GPSIFD = 26Bytes
                                   0x1A,0x00,0x00,0x00,
                                   //4 Byte offset to new segment -> GPS only segment
                                   0x00,0x00,0x00,0x00};
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

    QPair<int, int> getFraction(double d);

    /* Table of CRCs of all 8-bit messages. */
    unsigned long crc_table[256];

    /* Flag: has the table been computed? Initially false. */
    int crc_table_computed = 0;


    void make_crc_table();
    unsigned long update_crc(unsigned long crc, char *buf, int len);
    unsigned long crc(char *buf, int len);
};

#endif // EXPORTEXIF_H
