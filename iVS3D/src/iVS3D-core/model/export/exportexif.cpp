#include "exportexif.h"

ExportExif::ExportExif()
{
    TIFFHeader = new unsigned char[8];
    // II tag
    TIFFHeader[0] = 0x49;
    TIFFHeader[1] = 0x49;
    //Algin bytes 0x2A00
    TIFFHeader[2] = 0x2A;
    TIFFHeader[3] = 0x00;
    //Offset to first ifd (4 Bytes) --> total of 8 bytes offset
    TIFFHeader[4] = 0x08;
    TIFFHeader[5] = 0x00;
    TIFFHeader[6] = 0x00;
    TIFFHeader[7] = 0x00;

    JPEGMarker = new unsigned char[10];

    //APP1 Marker
    JPEGMarker[0] = 0xFF;
    JPEGMarker[1] = 0xE1;
    //Length of APP1 Field
    JPEGMarker[2] = 0x00;
    JPEGMarker[3] = 0xAA;
    //Exif
    JPEGMarker[4] = 0x45;
    JPEGMarker[5] = 0x78;
    JPEGMarker[6] = 0x69;
    JPEGMarker[7] = 0x66;
    //Offset
    JPEGMarker[8] = 0x00;
    JPEGMarker[9] = 0x00;
    JPEGMarker[10] = 0x00;

    PNGChunkHeader = new unsigned char[8];
    //4Bytes length of Data Only 172-12 = 160///////////(without PNGChunkHeader and crc) --> total length - 8B PNGChunkHeader - 4B CRC
    //TODO Size does change
    PNGChunkHeader[0] = 0x00;
    PNGChunkHeader[1] = 0x00;
    PNGChunkHeader[2] = 0x00;
    PNGChunkHeader[3] = 0x00;
    //eXIf Tag PNG --> eXIf
    PNGChunkHeader[4] = 101;
    PNGChunkHeader[5] = 88;
    PNGChunkHeader[6] = 73;
    PNGChunkHeader[7] = 102;
    GPSIFD = new unsigned char[18];
    //Number of fields in this IFD, 2 bytes --> only gps ifd
    GPSIFD[0] = 0x01;
    GPSIFD[1] = 0x00;
    //GPS IFD Tag, next 12Bytes is the IFD 12-byte field Interoperability array
    GPSIFD[2] = 0x25;
    GPSIFD[3] = 0x88;
    //Type -> long
    GPSIFD[4] = 0x04;
    GPSIFD[5] = 0x00;
    //Count
    GPSIFD[6] = 0x01;
    GPSIFD[7] = 0x00;
    GPSIFD[8] = 0x00;
    GPSIFD[9] = 0x00;
    //Value -> Pointer to gps tags 0x1A = 26: Offset from TIFF Header to GPSValues --> 8Byte TIFF + 18Bytes GPSIFD = 26Bytes
    GPSIFD[10] = 0x1A;
    GPSIFD[11] = 0x00;
    GPSIFD[12] = 0x00;
    GPSIFD[13] = 0x00;
    //4 Byte offset to new segment -> GPS only segment
    GPSIFD[14] = 0x00;
    GPSIFD[15] = 0x00;
    GPSIFD[16] = 0x00;
    GPSIFD[17] = 0x00;
}

char* ExportExif::saveExif(QString path, QVariant exif)
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
        return new char[0];
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
    int altitude;
    if (useAltitude) {
        altitudeRef = gpsHash.find(stringContainer::altitudeRefIdentifier).value().toString();
        altitudeRefChar = (altitudeRef == "0") ? 0 : 1;
        altitudeDouble = gpsHash.find(stringContainer::altitudeIdentifier).value().toDouble();
        altitude = int(altitudeDouble);
    }


    double degreeLatitude = int(latitude);
    double minutesLatitude = int((latitude - degreeLatitude) * 60);
    double secondsLatitude = int((latitude - degreeLatitude - (minutesLatitude/60)) * 3600);

    double degreeLongitude = int(longitude);
    double minutesLongitude= int((longitude - degreeLongitude) * 60);
    double secondsLongitude= int((longitude - degreeLongitude - (minutesLongitude/60)) * 3600);

    GPSTagCount = new unsigned char[2];
    //Count of new segment = 4 (Lat, LatRef, Long, LongRef) or 6 (.. + Alt, AltRef)
    GPSTagCount[0] = (useAltitude) ? 0x06 : 0x04;
    GPSTagCount[1] = 0x00;


    LatitudeRefTag = new unsigned char[12];
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


    LatitudeTag = new unsigned char[12];
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

    LongitudeRefTag = new unsigned char[12];
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


    LongitudeTag = new unsigned char[12];
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

    offestToGPSData = new unsigned char[4];
    //4 byte offset to data segment
    offestToGPSData[0] = 0x00;
    offestToGPSData[1] = 0x00;
    offestToGPSData[2] = 0x00;
    offestToGPSData[3] = 0x00;

    LatitudeData= new unsigned char[24];
    //Pointer to Latitude Data -> 3 rational with 2x4 bytes each -> second 4 byte block is always 1
    //Degree numerator
    LatitudeData[0] = degreeLatitude;
    LatitudeData[1] = 0x00;
    LatitudeData[2] = 0x00;
    LatitudeData[3] = 0x00;
    //Degree denumerator
    LatitudeData[4] = 0x01;
    LatitudeData[5] = 0x00;
    LatitudeData[6] = 0x00;
    LatitudeData[7] = 0x00;
    //Minutes numerator
    LatitudeData[8] = minutesLatitude;
    LatitudeData[9] = 0x00;
    LatitudeData[10] = 0x00;
    LatitudeData[11] = 0x00;
    //Minutes denumerator
    LatitudeData[12] = 0x01;
    LatitudeData[13] = 0x00;
    LatitudeData[14] = 0x00;
    LatitudeData[15] = 0x00;
    //Seconds numerator
    LatitudeData[16] = secondsLatitude;
    LatitudeData[17] = 0x00;
    LatitudeData[18] = 0x00;
    LatitudeData[19] = 0x00;
    //Seconds denumerator
    LatitudeData[20] = 0x01;
    LatitudeData[21] = 0x00;
    LatitudeData[22] = 0x00;
    LatitudeData[23] = 0x00;

    LongitudeData = new unsigned char[24];
    //Pointer to Longitude Data -> 3 rational with 2x4 bytes each -> second 4 byte block is always 1
    //Degree numerator
    LongitudeData[0] = degreeLongitude;
    LongitudeData[1] = 0x00;
    LongitudeData[2] = 0x00;
    LongitudeData[3] = 0x00;
    //Degree denumerator
    LongitudeData[4] = 0x01;
    LongitudeData[5] = 0x00;
    LongitudeData[6] = 0x00;
    LongitudeData[7] = 0x00;
    //Minutes numerator
    LongitudeData[8] = minutesLongitude;
    LongitudeData[9] = 0x00;
    LongitudeData[10] = 0x00;
    LongitudeData[11] = 0x00;
    //Minutes denumerator
    LongitudeData[12] = 0x01;
    LongitudeData[13] = 0x00;
    LongitudeData[14] = 0x00;
    LongitudeData[15] = 0x00;
    //Seconds numerator
    LongitudeData[16] = secondsLongitude;
    LongitudeData[17] = 0x00;
    LongitudeData[18] = 0x00;
    LongitudeData[19] = 0x00;
    //Seconds denumerator
    LongitudeData[20] = 0x01;
    LongitudeData[21] = 0x00;
    LongitudeData[22] = 0x00;
    LongitudeData[23] = 0x00;

    if (useAltitude) {
        AltitudeRefTag = new unsigned char[12];
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

        AltitudeTag = new unsigned char[12];
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

        AltitudeData = new unsigned char[8];
        //altitude numerator
        AltitudeData[0] = altitude;
        AltitudeData[1] = 0x00;
        AltitudeData[2] = 0x00;
        AltitudeData[3] = 0x00;
        //atitude denumerator
        AltitudeData[4] = 0x01;
        AltitudeData[5] = 0x00;
        AltitudeData[6] = 0x00;
        AltitudeData[7] = 0x00;

    }

    //End of Data
    PNGCRC = new unsigned char[4];
    //CRC over exifData --> without length field
    PNGCRC[0] = 0x00;//0x26;
    PNGCRC[1] = 0x00;//0x69;
    PNGCRC[2] = 0x00;//0x47;
    PNGCRC[3] = 0x00;//0x8B;

    if (useAltitude && usePNG) {
        //8Byte PNGChunk + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 6*12 GPSTags + 4B Data Offset + 2*24B + 8B Data + 4B CRC = 172B
        PNGChunkHeader[3] = 0xA0;
        exifSize = 172;
        char* newData = new char[exifSize];
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &GPSIFD[0], 18);
        std::memcpy(&newData[34], &GPSTagCount[0], 2);
        std::memcpy(&newData[36], &LatitudeRefTag[0], 12);
        std::memcpy(&newData[48], &LatitudeTag[0], 12);
        std::memcpy(&newData[60], &LongitudeRefTag[0], 12);
        std::memcpy(&newData[72], &LongitudeTag[0], 12);
        std::memcpy(&newData[84], &AltitudeRefTag[0], 12);
        std::memcpy(&newData[96], &AltitudeTag[0], 12);
        std::memcpy(&newData[108], &offestToGPSData[0], 4);
        std::memcpy(&newData[112], &LatitudeData[0], 24);
        std::memcpy(&newData[136], &LongitudeData[0], 24);
        std::memcpy(&newData[160], &AltitudeData[0], 8);
        std::memcpy(&newData[168], &PNGCRC[0], 4);
        return newData;
    }
    else if (useAltitude && !usePNG) {
        //10Byte JPEGMarker + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 6*12 GPSTags + 4B Data Offset + 2*24B + 8B Data = 170B
        //Set APP1 length
        JPEGMarker[3] = 0xAA;
        exifSize = 170;
        char* newData = new char[exifSize];
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &GPSIFD[0], 18);
        std::memcpy(&newData[36], &GPSTagCount[0], 2);
        std::memcpy(&newData[38], &LatitudeRefTag[0], 12);
        std::memcpy(&newData[50], &LatitudeTag[0], 12);
        std::memcpy(&newData[62], &LongitudeRefTag[0], 12);
        std::memcpy(&newData[74], &LongitudeTag[0], 12);
        std::memcpy(&newData[86], &AltitudeRefTag[0], 12);
        std::memcpy(&newData[98], &AltitudeTag[0], 12);
        std::memcpy(&newData[110], &offestToGPSData[0], 4);
        std::memcpy(&newData[114], &LatitudeData[0], 24);
        std::memcpy(&newData[138], &LongitudeData[0], 24);
        std::memcpy(&newData[162], &AltitudeData[0], 8);
        return newData;
    }
    else if (!useAltitude && usePNG) {
        //8Byte PNGChunk + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 4*12 GPSTags + 4B Data Offset + 2*24B + 4B CRC = 140B
        //Set png size
        PNGChunkHeader[3] = 0x80;
        exifSize = 140;
        char* newData = new char[exifSize];
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &GPSIFD[0], 18);
        std::memcpy(&newData[34], &GPSTagCount[0], 2);
        std::memcpy(&newData[36], &LatitudeRefTag[0], 12);
        std::memcpy(&newData[48], &LatitudeTag[0], 12);
        std::memcpy(&newData[60], &LongitudeRefTag[0], 12);
        std::memcpy(&newData[72], &LongitudeTag[0], 12);
        std::memcpy(&newData[84], &offestToGPSData[0], 4);
        std::memcpy(&newData[88], &LatitudeData[0], 24);
        std::memcpy(&newData[112], &LongitudeData[0], 24);
        std::memcpy(&newData[136], &PNGCRC[0], 4);
        return newData;
    }

    else if (!useAltitude && !usePNG) {
        //10Byte JPEGMarker + 8B TIFFHeader + 18B GPSIFD + 2B GPSTagCount + 4*12 GPSTags + 4B Data Offset + 2*24B = 138B
        //Set APP1 length
        JPEGMarker[3] = 0x8A;
        exifSize = 138;
        char* newData = new char[exifSize];
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &GPSIFD[0], 18);
        std::memcpy(&newData[36], &GPSTagCount[0], 2);
        std::memcpy(&newData[38], &LatitudeRefTag[0], 12);
        std::memcpy(&newData[50], &LatitudeTag[0], 12);
        std::memcpy(&newData[62], &LongitudeRefTag[0], 12);
        std::memcpy(&newData[74], &LongitudeTag[0], 12);
        std::memcpy(&newData[86], &offestToGPSData[0], 4);
        std::memcpy(&newData[90], &LatitudeData[0], 24);
        std::memcpy(&newData[114], &LongitudeData[0], 24);

        return newData;
    }




}

int ExportExif::getExifSize()
{
    return exifSize;
}
