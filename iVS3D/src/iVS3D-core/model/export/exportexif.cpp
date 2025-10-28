#include "exportexif.h"
#include "qdebug.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numeric>

static inline void put_u32_le(unsigned char *p, uint32_t v)
{
    p[0] = static_cast<unsigned char>(v & 0xFF);
    p[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
    p[2] = static_cast<unsigned char>((v >> 16) & 0xFF);
    p[3] = static_cast<unsigned char>((v >> 24) & 0xFF);
}

static inline uint32_t clamp_u32(int64_t v)
{
    if (v < 0)
        v = 0;
    if (v > 0xFFFFFFFFLL)
        v = 0xFFFFFFFFLL;
    return static_cast<uint32_t>(v);
}

static void toDMS(double absDeg, uint32_t &deg, uint32_t &min, double &sec)
{
    double d = std::floor(absDeg + 1e-15);
    double remMin = (absDeg - d) * 60.0;
    double m = std::floor(remMin + 1e-15);
    double s = (remMin - m) * 60.0;

    // Round seconds to microseconds and apply carry if needed
    double sRounded = std::round(s * 1'000'000.0) / 1'000'000.0;
    if (sRounded >= 60.0) {
        sRounded -= 60.0;
        m += 1.0;
    }
    if (m >= 60.0) {
        m -= 60.0;
        d += 1.0;
    }

    deg = static_cast<uint32_t>(d);
    min = static_cast<uint32_t>(m);
    sec = sRounded;
}

std::vector<char> ExportExif::saveExif(QString path, QVariant exif)
{
    QFileInfo info(path);
    QString fileExtension = info.completeSuffix().toLower();
    if (fileExtension == "jpg") {
        usePNG = false;
    } else if (fileExtension == "png") {
        usePNG = true;
    } else {
        return std::vector<char>();
    }

    QHash<QString, QVariant> gpsHash = exif.toHash();
    useAltitude = gpsHash.find(stringContainer::altitudeIdentifier) != gpsHash.end();

    // Read decimals
    double latitude = gpsHash.find(stringContainer::latitudeIdentifier).value().toDouble();
    double longitude = gpsHash.find(stringContainer::longitudeIdentifier).value().toDouble();

    // Determine refs; if missing, derive from sign
    QString latitudeRef = gpsHash.value(stringContainer::latitudeRefIdentifier).toString();
    if (latitudeRef.isEmpty())
        latitudeRef = (latitude < 0.0) ? "S" : "N";
    QString longitudeRef = gpsHash.value(stringContainer::longitudeRefIdentifier).toString();
    if (longitudeRef.isEmpty())
        longitudeRef = (longitude < 0.0) ? "W" : "E";

    QByteArray latRefBytes = latitudeRef.toLatin1();
    QByteArray lonRefBytes = longitudeRef.toLatin1();
    unsigned char latitudeRefChar = latRefBytes.isEmpty()
                                        ? static_cast<unsigned char>((latitude < 0.0) ? 'S' : 'N')
                                        : static_cast<unsigned char>(latRefBytes[0]);
    unsigned char longitudeRefChar = lonRefBytes.isEmpty()
                                         ? static_cast<unsigned char>((longitude < 0.0) ? 'W' : 'E')
                                         : static_cast<unsigned char>(lonRefBytes[0]);

    // Altitude (unsigned rational; sign via AltitudeRef)
    QString altitudeRef;
    int altitudeRefChar = 0;
    double altitudeDouble = 0.0;
    QPair<int, int> altitude = QPair<int, int>(0, 1);
    if (useAltitude) {
        altitudeRef = gpsHash.find(stringContainer::altitudeRefIdentifier).value().toString();
        altitudeRefChar = (altitudeRef == "0") ? 0 : 1;
        altitudeDouble = gpsHash.find(stringContainer::altitudeIdentifier).value().toDouble();
        altitude = getFraction(std::fabs(altitudeDouble)); // store positive; ref carries sign
    }

    // Convert to DMS using absolute values
    double absLat = std::fabs(latitude);
    double absLon = std::fabs(longitude);

    uint32_t degLat = 0, minLat = 0;
    double secLatD = 0.0;
    toDMS(absLat, degLat, minLat, secLatD);

    uint32_t degLon = 0, minLon = 0;
    double secLonD = 0.0;
    toDMS(absLon, degLon, minLon, secLonD);

    // Fractions for seconds
    QPair<int, int> secondsLatitude_fraction = getFraction(secLatD);
    QPair<int, int> secondsLongitude_fraction = getFraction(secLonD);

    unsigned char GPSTagCount[2];
    // Count: 4 (Lat, LatRef, Lon, LonRef) or 6 (+ Alt, AltRef)
    GPSTagCount[0] = (useAltitude) ? 0x06 : 0x04;
    GPSTagCount[1] = 0x00;

    unsigned char LatitudeRefTag[12];
    // GPS LatitudeRef -> tag 0x0001, ASCII, count 2, value "N\0" or "S\0"
    LatitudeRefTag[0] = 0x01;
    LatitudeRefTag[1] = 0x00;
    LatitudeRefTag[2] = 0x02;
    LatitudeRefTag[3] = 0x00;
    LatitudeRefTag[4] = 0x02;
    LatitudeRefTag[5] = 0x00;
    LatitudeRefTag[6] = 0x00;
    LatitudeRefTag[7] = 0x00;
    LatitudeRefTag[8] = latitudeRefChar;
    LatitudeRefTag[9] = 0x00;
    LatitudeRefTag[10] = 0x00;
    LatitudeRefTag[11] = 0x00;

    unsigned char LatitudeTag[12];
    // GPS Latitude -> tag 0x0002, RATIONAL, count 3, data offset
    LatitudeTag[0] = 0x02;
    LatitudeTag[1] = 0x00;
    LatitudeTag[2] = 0x05;
    LatitudeTag[3] = 0x00;
    LatitudeTag[4] = 0x03;
    LatitudeTag[5] = 0x00;
    LatitudeTag[6] = 0x00;
    LatitudeTag[7] = 0x00;
    LatitudeTag[8] = (useAltitude) ? 0x68 : 0x50; // data offset (relative to TIFF header start)
    LatitudeTag[9] = 0x00;
    LatitudeTag[10] = 0x00;
    LatitudeTag[11] = 0x00;

    unsigned char LongitudeRefTag[12];
    // GPS LongitudeRef -> tag 0x0003, ASCII, count 2, value "E\0" or "W\0"
    LongitudeRefTag[0] = 0x03;
    LongitudeRefTag[1] = 0x00;
    LongitudeRefTag[2] = 0x02;
    LongitudeRefTag[3] = 0x00;
    LongitudeRefTag[4] = 0x02;
    LongitudeRefTag[5] = 0x00;
    LongitudeRefTag[6] = 0x00;
    LongitudeRefTag[7] = 0x00;
    LongitudeRefTag[8] = longitudeRefChar;
    LongitudeRefTag[9] = 0x00;
    LongitudeRefTag[10] = 0x00;
    LongitudeRefTag[11] = 0x00;

    unsigned char LongitudeTag[12];
    // GPS Longitude -> tag 0x0004, RATIONAL, count 3, data offset
    LongitudeTag[0] = 0x04;
    LongitudeTag[1] = 0x00;
    LongitudeTag[2] = 0x05;
    LongitudeTag[3] = 0x00;
    LongitudeTag[4] = 0x03;
    LongitudeTag[5] = 0x00;
    LongitudeTag[6] = 0x00;
    LongitudeTag[7] = 0x00;
    LongitudeTag[8] = (useAltitude) ? 0x80 : 0x68; // data offset (relative to TIFF header start)
    LongitudeTag[9] = 0x00;
    LongitudeTag[10] = 0x00;
    LongitudeTag[11] = 0x00;

    unsigned char offestToGPSData[4] = {0x00, 0x00, 0x00, 0x00};

    unsigned char LatitudeData[24];
    // Degrees
    put_u32_le(&LatitudeData[0], degLat);
    put_u32_le(&LatitudeData[4], 1);
    // Minutes
    put_u32_le(&LatitudeData[8], minLat);
    put_u32_le(&LatitudeData[12], 1);
    // Seconds
    put_u32_le(&LatitudeData[16], static_cast<uint32_t>(secondsLatitude_fraction.first));
    put_u32_le(&LatitudeData[20], static_cast<uint32_t>(secondsLatitude_fraction.second));

    unsigned char LongitudeData[24];
    // Degrees
    put_u32_le(&LongitudeData[0], degLon);
    put_u32_le(&LongitudeData[4], 1);
    // Minutes
    put_u32_le(&LongitudeData[8], minLon);
    put_u32_le(&LongitudeData[12], 1);
    // Seconds
    put_u32_le(&LongitudeData[16], static_cast<uint32_t>(secondsLongitude_fraction.first));
    put_u32_le(&LongitudeData[20], static_cast<uint32_t>(secondsLongitude_fraction.second));

    unsigned char PNGChunkHeader[8] = {
        0x00,
        0x00,
        0x00,
        0x00, // length (to be set)
        101,
        88,
        73,
        102 // "eXIf"
    };
    unsigned char JPEGMarker[10] = {
        0xFF,
        0xE1,
        0x00,
        0x00, // length (to be set)
        0x45,
        0x78,
        0x69,
        0x66, // "Exif"
        0x00,
        0x00 // \0\0
    };

    unsigned char *exifData;
    if (useAltitude) {
        unsigned char AltitudeRefTag[12];
        // GPS AltitudeRef -> tag 0x0005, BYTE, count 1
        AltitudeRefTag[0] = 0x05;
        AltitudeRefTag[1] = 0x00;
        AltitudeRefTag[2] = 0x01;
        AltitudeRefTag[3] = 0x00;
        AltitudeRefTag[4] = 0x01;
        AltitudeRefTag[5] = 0x00;
        AltitudeRefTag[6] = 0x00;
        AltitudeRefTag[7] = 0x00;
        AltitudeRefTag[8] = static_cast<unsigned char>(altitudeRefChar);
        AltitudeRefTag[9] = 0x00;
        AltitudeRefTag[10] = 0x00;
        AltitudeRefTag[11] = 0x00;

        unsigned char AltitudeTag[12];
        // GPS Altitude -> tag 0x0006, RATIONAL, count 1
        AltitudeTag[0] = 0x06;
        AltitudeTag[1] = 0x00;
        AltitudeTag[2] = 0x05;
        AltitudeTag[3] = 0x00;
        AltitudeTag[4] = 0x01;
        AltitudeTag[5] = 0x00;
        AltitudeTag[6] = 0x00;
        AltitudeTag[7] = 0x00;
        // Data offset: 8 TIFF + 18 GPSIFD + 2 count + 6*12 tags + 4 nextIFD + 24 Lat + 24 Lon = 152 bytes
        AltitudeTag[8] = 0x98;
        AltitudeTag[9] = 0x00;
        AltitudeTag[10] = 0x00;
        AltitudeTag[11] = 0x00;

        unsigned char AltitudeData[8];
        put_u32_le(&AltitudeData[0], static_cast<uint32_t>(altitude.first));
        put_u32_le(&AltitudeData[4], static_cast<uint32_t>(altitude.second));

        // 18 + 2 + 6*12 + 4 + 24 + 24 + 8 = 152
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
    } else {
        // 18 + 2 + 4*12 + 4 + 24 + 24 = 120
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

    if (useAltitude && usePNG) {
        // PNG eXIf dataLen = 8 (TIFF) + 152 (exif) = 160
        const uint32_t dataLen = 160;
        PNGChunkHeader[0] = 0x00;
        PNGChunkHeader[1] = 0x00;
        PNGChunkHeader[2] = 0x00;
        PNGChunkHeader[3] = 0xA0;
        exifSize = 8 /*chunk header*/ + dataLen + 4 /*CRC*/; // 172
        newData.resize(exifSize);
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &exifData[0], 152);

        // CRC over chunk type + data
        unsigned long crcLong = crc(&newData[4], 4 + dataLen);
        newData[8 + dataLen] = static_cast<unsigned char>((crcLong >> 24) & 0xFF);
        newData[8 + dataLen + 1] = static_cast<unsigned char>((crcLong >> 16) & 0xFF);
        newData[8 + dataLen + 2] = static_cast<unsigned char>((crcLong >> 8) & 0xFF);
        newData[8 + dataLen + 3] = static_cast<unsigned char>(crcLong & 0xFF);
    } else if (useAltitude && !usePNG) {
        // JPEG APP1 total inserted = 10 marker + 8 TIFF + 152 exif = 170
        const uint16_t app1Len = static_cast<uint16_t>(6 /*Exif\0\0*/ + 8 /*TIFF*/ + 152); // 166
        JPEGMarker[2] = static_cast<unsigned char>((app1Len >> 8) & 0xFF);
        JPEGMarker[3] = static_cast<unsigned char>(app1Len & 0xFF);
        exifSize = 10 + 8 + 152; // 170
        newData.resize(exifSize);
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &exifData[0], 152);
    } else if (!useAltitude && usePNG) {
        // PNG eXIf dataLen = 8 (TIFF) + 120 (exif) = 128
        const uint32_t dataLen = 128;
        PNGChunkHeader[0] = 0x00;
        PNGChunkHeader[1] = 0x00;
        PNGChunkHeader[2] = 0x00;
        PNGChunkHeader[3] = 0x80;
        exifSize = 8 /*chunk header*/ + dataLen + 4 /*CRC*/; // 140
        newData.resize(exifSize);
        std::memcpy(&newData[0], &PNGChunkHeader[0], 8);
        std::memcpy(&newData[8], &TIFFHeader[0], 8);
        std::memcpy(&newData[16], &exifData[0], 120);

        // CRC over chunk type + data
        unsigned long crcLong = crc(&newData[4], 4 + dataLen);
        newData[8 + dataLen] = static_cast<unsigned char>((crcLong >> 24) & 0xFF);
        newData[8 + dataLen + 1] = static_cast<unsigned char>((crcLong >> 16) & 0xFF);
        newData[8 + dataLen + 2] = static_cast<unsigned char>((crcLong >> 8) & 0xFF);
        newData[8 + dataLen + 3] = static_cast<unsigned char>(crcLong & 0xFF);
    } else { // !useAltitude && !usePNG
        // JPEG APP1 total inserted = 10 marker + 8 TIFF + 120 exif = 138
        const uint16_t app1Len = static_cast<uint16_t>(6 /*Exif\0\0*/ + 8 /*TIFF*/ + 120); // 134
        JPEGMarker[2] = static_cast<unsigned char>((app1Len >> 8) & 0xFF);
        JPEGMarker[3] = static_cast<unsigned char>(app1Len & 0xFF);
        exifSize = 10 + 8 + 120; // 138
        newData.resize(exifSize);
        std::memcpy(&newData[0], &JPEGMarker[0], 10);
        std::memcpy(&newData[10], &TIFFHeader[0], 8);
        std::memcpy(&newData[18], &exifData[0], 120);
    }

    delete[] exifData;
    return newData;
}

// Returns an unsigned rational approximation of d.
// Uses higher precision (1e6) for values < 1000 (e.g., seconds),
// and lower precision (1e4) for large values (e.g., altitude) to avoid overflow.
QPair<int, int> ExportExif::getFraction(double d)
{
    double ad = std::fabs(d);
    double floorPart = std::floor(ad);
    double frac = ad - floorPart;

    // Choose precision adaptively
    int64_t precision = (ad < 1000.0) ? 1000000LL : 10000LL;

    int64_t fracNum = static_cast<int64_t>(std::llround(frac * precision));
    int64_t g = std::gcd(fracNum, precision);
    int64_t den = precision / g;
    int64_t num = static_cast<int64_t>(floorPart) * den + (fracNum / g);

    // Clamp to 32-bit unsigned range (EXIF RATIONAL fields are 32-bit each)
    if (num > 0xFFFFFFFFLL || den > 0xFFFFFFFFLL) {
        // Reduce by stripping common factors of 2 or 5 to try to fit
        auto reduce_factor = [](int64_t &a, int64_t &b, int factor) {
            while (a % factor == 0 && b % factor == 0) {
                a /= factor;
                b /= factor;
            }
        };
        reduce_factor(num, den, 2);
        reduce_factor(num, den, 5);
        if (num > 0xFFFFFFFFLL || den > 0xFFFFFFFFLL) {
            // As a last resort, scale down denominator to fit
            double scale = std::max((num / static_cast<double>(0xFFFFFFFFu)) + 1.0,
                                    (den / static_cast<double>(0xFFFFFFFFu)) + 1.0);
            den = static_cast<int64_t>(std::ceil(den / scale));
            num = static_cast<int64_t>(std::llround(ad * den));
        }
    }

    int den32 = static_cast<int>(clamp_u32(den));
    int num32 = static_cast<int>(clamp_u32(num));
    return QPair<int, int>(num32, den32);
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

unsigned long ExportExif::update_crc(unsigned long crc, char *buf, int len)
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
