#include "exportexif.h"
#include "qdebug.h"
#include <cstdio>
#include <iostream>

std::vector<char> ExportExif::saveExif(QString path, QVariant exif)
{
    QFileInfo info(path);
    QString fileExtension = info.completeSuffix().toLower();
    if (fileExtension == "jpeg" || fileExtension == "jpg") {
        usePNG = false;
    }
    else if (fileExtension == "png") {
        usePNG = true;
    }
    else {
        return std::vector<char>();
    }


    QHash<QString, QVariant> gpsHash = exif.toHash();
    useAltitude = gpsHash.find(stringContainer::altitudeIdentifier) != gpsHash.end();

    double latitude = gpsHash.find(stringContainer::latitudeIdentifier).value().toDouble();
    double longitude = gpsHash.find(stringContainer::longitudeIdentifier).value().toDouble();


    QString latitudeRef = gpsHash.find(stringContainer::latitudeRefIdentifier).value().toString();
    const unsigned char* latitudeRefChar = reinterpret_cast<const unsigned char *>(latitudeRef.constData());
    QString longitudeRef = gpsHash.find(stringContainer::longitudeRefIdentifier).value().toString();
    const unsigned char* longitudeRefChar = reinterpret_cast<const unsigned char *>(longitudeRef.constData());

    QString altitudeRef;
    int altitudeRefChar;
    double altitudeDouble;
    QPair<int, int>  altitude;
    if (useAltitude) {
        altitudeRef = gpsHash.find(stringContainer::altitudeRefIdentifier).value().toString();
        altitudeRefChar = (altitudeRef == "0") ? 0 : 1;
        altitudeDouble = gpsHash.find(stringContainer::altitudeIdentifier).value().toDouble();
        altitude = getFraction(altitudeDouble);
    }

    double d =  int(latitude);
    double min = int((latitude-d) * 60);
    double sec = (((latitude-d) * 60) - min) * 60;


    double degreeLatitude = int(latitude);
    double minutesLatitude = int((latitude - degreeLatitude) * 60);
    double secondsLatitude = (((latitude-degreeLatitude) * 60) - minutesLatitude) * 60;

    double degreeLongitude = int(longitude);
    double minutesLongitude= int((longitude - degreeLongitude) * 60);
    double secondsLongitude= (((longitude-degreeLongitude) * 60) - minutesLongitude) * 60;

    QPair<int, int> secondsLatitude_fraction  = getFraction(secondsLatitude);
    QPair<int, int> secondsLongitude_fraction = getFraction(secondsLongitude);



    unsigned char GPSTagCount[2];
    //Count of new segment = 4 (Lat, LatRef, Long, LongRef) or 6 (.. + Alt, AltRef)
    GPSTagCount[0] = (useAltitude) ? 0x06 : 0x04;
    GPSTagCount[1] = 0x00;


    unsigned char LatitudeRefTag[12];
    //GPS LatitudeRef -> 12 bytes, tag 01
    LatitudeRefTag[0] = 0x01;
    LatitudeRefTag[1] = 0x00;
    //Ascii type
    LatitudeRefTag[2] = 0x02;
    LatitudeRefTag[3] = 0x00;
    //Count 2
    LatitudeRefTag[4] = 0x02;
    LatitudeRefTag[5] = 0x00;
    LatitudeRefTag[6] = 0x00;
    LatitudeRefTag[7] = 0x00;
    //Value -> N or S
    LatitudeRefTag[8] = latitudeRefChar[0];
    LatitudeRefTag[9] = 0x00;
    LatitudeRefTag[10] = 0x00;
    LatitudeRefTag[11] = 0x00;


    unsigned char LatitudeTag[12];
    //GPS Latitude -> 12 bytes, tag 02
    LatitudeTag[0] = 0x02;
    LatitudeTag[1] = 0x00;
    //rational type
    LatitudeTag[2] = 0x05;
    LatitudeTag[3] = 0x00;
    //Count 3 (deg,h,s)
    LatitudeTag[4] = 0x03;
    LatitudeTag[5] = 0x00;
    LatitudeTag[6] = 0x00;
    LatitudeTag[7] = 0x00;
    //Pointer to actual values -> with altitude: 8Byte TIFFHeader + 18Bytes GPSIFD + 2Bytes GPSTagCount + 6*12Bytes GPSTags + 4Byte Data Offset = 104Bytes
    //Pointer without altitude = 104Bytes - 12Byte AltTag - 12Byte AltRefTag = 80Bytes
    LatitudeTag[8] = (useAltitude) ? 0x68 : 0x50;
    LatitudeTag[9] = 0x00;
    LatitudeTag[10] = 0x00;
    LatitudeTag[11] = 0x00;

    unsigned char LongitudeRefTag[12];
    //GPS LongitudeRef -> 12 bytes, tag 03
    LongitudeRefTag[0] = 0x03;
    LongitudeRefTag[1] = 0x00;
    //Ascii type
    LongitudeRefTag[2] = 0x02;
    LongitudeRefTag[3] = 0x00;
    //Count 2
    LongitudeRefTag[4] = 0x02;
    LongitudeRefTag[5] = 0x00;
    LongitudeRefTag[6] = 0x00;
    LongitudeRefTag[7] = 0x00;
    //Value -> E or W
    LongitudeRefTag[8] = longitudeRefChar[0];
    LongitudeRefTag[9] = 0x00;
    LongitudeRefTag[10] = 0x00;
    LongitudeRefTag[11] = 0x00;


    unsigned char LongitudeTag[12];
    //GPS Longitude -> 12 bytes, tag 04
    LongitudeTag[0] = 0x04;
    LongitudeTag[1] = 0x00;
    //Rational type
    LongitudeTag[2] = 0x05;
    LongitudeTag[3] = 0x00;
    //Count 3 (deg,h,s)
    LongitudeTag[4] = 0x03;
    LongitudeTag[5] = 0x00;
    LongitudeTag[6] = 0x00;
    LongitudeTag[7] = 0x00;
    //Pointer to actual values -> with altitude: 8Byte TIFFHeader + 18Bytes GPSIFD + 2Bytes GPSTagCount + 6*12Bytes GPSTags + 4Byte Data Offset + 24 Byte Latitude Data = 128Bytes
    //Pointer without altitude = 128Bytes - 12Byte AltTag - 12Byte AltRefTag = 104Bytes
    LongitudeTag[8] = (useAltitude) ? 0x80 : 0x68;
    LongitudeTag[9] = 0x00;
    LongitudeTag[10] = 0x00;
    LongitudeTag[11] = 0x00;

    unsigned char offestToGPSData[4];
    //4 byte offset to data segment
    offestToGPSData[0] = 0x00;
    offestToGPSData[1] = 0x00;
    offestToGPSData[2] = 0x00;
    offestToGPSData[3] = 0x00;

    unsigned char LatitudeData[24];
    //Pointer to Latitude Data -> 3 rational with 2x4 bytes each
    //Degree numerator
    LatitudeData[0] = degreeLatitude;
    LatitudeData[1] = 0x00;
    LatitudeData[2] = 0x00;
    LatitudeData[3] = 0x00;
    //Degree denumerator
    LatitudeData[4] = 1;
    LatitudeData[5] = 0x00;
    LatitudeData[6] = 0x00;
    LatitudeData[7] = 0x00;
    //Minutes numerator
    LatitudeData[8] = minutesLatitude;
    LatitudeData[9] = 0x00;
    LatitudeData[10] = 0x00;
    LatitudeData[11] = 0x00;
    //Minutes denumerator
    LatitudeData[12] = 1;
    LatitudeData[13] = 0x00;
    LatitudeData[14] = 0x00;
    LatitudeData[15] = 0x00;
    //Seconds numerator
    LatitudeData[16] = secondsLatitude_fraction.first & 0xFF;
    LatitudeData[17] = (secondsLatitude_fraction.first >> 8) & 0xFF;
    LatitudeData[18] = (secondsLatitude_fraction.first >> 16) & 0xFF;
    LatitudeData[19] = (secondsLatitude_fraction.first >> 24) & 0xFF;
    //Seconds denumerator
    LatitudeData[20] =  secondsLatitude_fraction.second & 0xFF;
    LatitudeData[21] =  (secondsLatitude_fraction.second >> 8) & 0xFF;
    LatitudeData[22] =  (secondsLatitude_fraction.second >> 16) & 0xFF;
    LatitudeData[23] =  (secondsLatitude_fraction.second >> 24) & 0xFF;

    unsigned char LongitudeData[24];
    //Pointer to Longitude Data -> 3 rational with 2x4 bytes each -> second 4 byte block is always 1
    //Degree numerator
    LongitudeData[0] = degreeLongitude;
    LongitudeData[1] = 0x00;
    LongitudeData[2] = 0x00;
    LongitudeData[3] = 0x00;
    //Degree denumerator
    LongitudeData[4] = 1;
    LongitudeData[5] = 0x00;
    LongitudeData[6] = 0x00;
    LongitudeData[7] = 0x00;
    //Minutes numerator
    LongitudeData[8] = minutesLongitude;
    LongitudeData[9] = 0x00;
    LongitudeData[10] = 0x00;
    LongitudeData[11] = 0x00;
    //Minutes denumerator
    LongitudeData[12] = 1;
    LongitudeData[13] = 0x00;
    LongitudeData[14] = 0x00;
    LongitudeData[15] = 0x00;
    //Seconds numerator
    LongitudeData[16] = secondsLongitude_fraction.first & 0xFF;
    LongitudeData[17] = (secondsLongitude_fraction.first >> 8) & 0xFF;
    LongitudeData[18] = (secondsLongitude_fraction.first >> 16) & 0xFF;
    LongitudeData[19] = (secondsLongitude_fraction.first >> 24) & 0xFF;
    //Seconds denumerator
    LongitudeData[20] = secondsLongitude_fraction.second & 0xFF;
    LongitudeData[21] = (secondsLongitude_fraction.second >> 8) & 0xFF;
    LongitudeData[22] = (secondsLongitude_fraction.second >> 16) & 0xFF;
    LongitudeData[23] = (secondsLongitude_fraction.second >> 24) & 0xFF;


    unsigned char PNGChunkHeader[8] = { 0x00,0x00,0x00,0x00,
                                              //eXIf Tag PNG --> eXIf
                                              101,88,73,102 };
    unsigned char JPEGMarker[10] = { 0xFF,0xE1,
                                          //Length of APP1 Field
                                          0x00,0xAA,
                                          //Exif
                                          0x45,0x78,0x69,0x66,
                                          //Offset
                                          0x00,0x00 };
    unsigned char* exifData;
    if (useAltitude) {
        unsigned char AltitudeRefTag[12];
        //GPS AltitudeRefTag -> 12 bytes, tag 05
        AltitudeRefTag[0] = 0x05;
        AltitudeRefTag[1] = 0x00;
        //Byte type
        AltitudeRefTag[2] = 0x01;
        AltitudeRefTag[3] = 0x00;
        //Count 1
        AltitudeRefTag[4] = 0x01;
        AltitudeRefTag[5] = 0x00;
        AltitudeRefTag[6] = 0x00;
        AltitudeRefTag[7] = 0x00;
        //Value -> 0 or 1
        AltitudeRefTag[8] = altitudeRefChar;
        AltitudeRefTag[9] = 0x00;
        AltitudeRefTag[10] = 0x00;
        AltitudeRefTag[11] = 0x00;

        unsigned char AltitudeTag[12];
        //GPS AltitudeTag -> 12 bytes, tag 06
        AltitudeTag[0] = 0x06;
        AltitudeTag[1] = 0x00;
        //Rational type
        AltitudeTag[2] = 0x05;
        AltitudeTag[3] = 0x00;
        //Count 1
        AltitudeTag[4] = 0x01;
        AltitudeTag[5] = 0x00;
        AltitudeTag[6] = 0x00;
        AltitudeTag[7] = 0x00;
        //Pointer to Altitude Data:  8Byte TIFFHeader + 18Bytes GPSIFD + 2Bytes GPSTagCount + 6*12Bytes GPSTags + 4Byte Data Offset + 24 Byte Latitude Data + 24Byte Longitude Data = 152Bytes
        AltitudeTag[8] = 0x98;
        AltitudeTag[9] = 0x00;
        AltitudeTag[10] = 0x00;
        AltitudeTag[11] = 0x00;

        unsigned char AltitudeData[8];
        //altitude numerator
        AltitudeData[0] = altitude.first & 0xFF;
        AltitudeData[1] = (altitude.first >> 8) & 0xFF;
        AltitudeData[2] = (altitude.first >> 16) & 0xFF;
        AltitudeData[3] = (altitude.first >> 24) & 0xFF;
        //atitude denumerator
        AltitudeData[4] = altitude.second & 0xFF;
        AltitudeData[5] = (altitude.second >> 8) & 0xFF;
        AltitudeData[6] = (altitude.second >> 16) & 0xFF;
        AltitudeData[7] = (altitude.second >> 24) & 0xFF;

        //18B GPSIFD + 2B GPSTagCount + 6*12 GPSTags + 4B Data Offset + 2*24B + 8B Data = 152B
        exifData = new unsigned char[152];
        std::memcpy(&exifData[0], &GPSIFD[0], 18);
        std::memcpy(&exifData[18], &GPSTagCount[0], 2);
        std::memcpy(&exifData[20], &LatitudeRefTag[0], 12);
        std::memcpy(&exifData[32], &LatitudeTag[0], 12);
        std::memcpy(&exifData[44], &LongitudeRefTag[0], 12);
        std::memcpy(&exifData[56], &LongitudeTag[0], 12);
        std::memcpy(&exifData[68], &AltitudeRefTag[0], 12);
        std::memcpy(&exifData[80], &AltitudeTag[0], 12);
        std::memcpy(&exifData[92], &offestToGPSData[0], 4);
        std::memcpy(&exifData[96], &LatitudeData[0], 24);
        std::memcpy(&exifData[120], &LongitudeData[0], 24);
        std::memcpy(&exifData[144], &AltitudeData[0], 8);
    }
    else {
        //18B GPSIFD + 2B GPSTagCount + 4*12 GPSTags + 4B Data Offset + 2*24B = 120
        exifData = new unsigned char[120];
        std::memcpy(&exifData[0], &GPSIFD[0], 18);
        std::memcpy(&exifData[18], &GPSTagCount[0], 2);
        std::memcpy(&exifData[20], &LatitudeRefTag[0], 12);
        std::memcpy(&exifData[32], &LatitudeTag[0], 12);
        std::memcpy(&exifData[44], &LongitudeRefTag[0], 12);
        std::memcpy(&exifData[56], &LongitudeTag[0], 12);
        std::memcpy(&exifData[68], &offestToGPSData[0], 4);
        std::memcpy(&exifData[72], &LatitudeData[0], 24);
        std::memcpy(&exifData[96], &LongitudeData[0], 24);
    }
    std::vector<char> newData;
    unsigned char* PNGCRC = new unsigned char[4];

    if (useAltitude && usePNG) {
        //8Byte PNGChunk + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 6*12 GPSTags + 4B Data Offset + 2*24B + 8B Data + 4B CRC = 172B
        PNGChunkHeader[3] = 0xA0;
        exifSize = 172;
        newData.resize(exifSize);
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &exifData[0], 152);

        //CRC over exifData --> without length field
        unsigned long crcLong = crc(&newData[4], 164);
        PNGCRC[0] = (crcLong >> 24) & 0xFF;
        PNGCRC[1] = (crcLong >> 16) & 0xFF;
        PNGCRC[2] = (crcLong >> 8) & 0xFF;
        PNGCRC[3] = crcLong & 0xFF;
        std::memcpy(&newData[168], &PNGCRC[0], 4);
    }
    else if (useAltitude && !usePNG) {
        //10Byte JPEGMarker + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 6*12 GPSTags + 4B Data Offset + 2*24B + 8B Data = 170B
        //Set APP1 length
        JPEGMarker[3] = 0xAA;
        exifSize = 170;
        newData.resize(exifSize);
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &exifData[0], 152);
    }
    else if (!useAltitude && usePNG) {
        //8Byte PNGChunk + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 4*12 GPSTags + 4B Data Offset + 2*24B + 4B CRC = 140B
        //Set png size
        PNGChunkHeader[3] = 0x80;
        exifSize = 140;
        newData.resize(exifSize);
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &exifData[0], 120);

        PNGCRC = new unsigned char[4];
        //CRC over exifData --> without length field
        unsigned long crcLong = crc(&newData[4], 164);
        PNGCRC[0] = (crcLong >> 24) & 0xFF;
        PNGCRC[1] = (crcLong >> 16) & 0xFF;
        PNGCRC[2] = (crcLong >> 8) & 0xFF;
        PNGCRC[3] = crcLong & 0xFF;
        std::memcpy(&newData[168], &PNGCRC[0], 4);
    }

    else if (!useAltitude && !usePNG) {
        //10Byte JPEGMarker + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 4*12 GPSTags + 4B Data Offset + 2*24B = 138B
        //Set APP1 length
        JPEGMarker[3] = 0x8A;
        exifSize = 138;
        newData.resize(exifSize);
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &exifData[0], 120);
    }
    delete[] exifData;
    delete[] PNGCRC;
//    std::vector<char> newData_vec(newData,newData+exifSize);
    return newData;
}

QPair<int, int> ExportExif::getFraction(double d)
{
    double floor = qFloor(d);
    //get decimal places
    double decimal = d - floor;
    int precision = 10000000;
    //get greatest common divisor
    int divisor = std::gcd(qRound(decimal * precision), precision);
    //divise to get numerator and denumerator
    int division = (decimal * precision) / divisor;
    int denumerator = precision / divisor;
    int numerator = floor * denumerator + division;
    return QPair<int, int>(numerator, denumerator);
}



/* Make the table for a fast CRC. */
void ExportExif::make_crc_table()
{
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long) n;
    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

unsigned long ExportExif::update_crc(unsigned long crc, char *buf,
                         int len)
{
  unsigned long c = crc;
  int n;

  if (!crc_table_computed)
    make_crc_table();
  for (n = 0; n < len; n++) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long ExportExif::crc(char *buf, int len)
{
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}



