#include "gpsreaderexif.h"
#include <QDebug>

GPSReaderExif::GPSReaderExif()
{

}

GPSReaderExif::~GPSReaderExif()
{
    m_GPSHashs.clear();
}

QString GPSReaderExif::getName()
{
    return m_name;
}


bool GPSReaderExif::parseDataImage(std::vector<std::string> paths)
{
    int gapSize = 0;
    QList<int> missingMetaData;
    int index = -1;
    for (std::string path : paths) {
        index++;
        // https://github.com/mayanklahiri/easyexif/blob/master/demo.cpp
        // Read the JPEG file into a buffer
        FILE *fp = std::fopen(path.data(), "rb");
        if (!fp) {
          return false;
        }
        fseek(fp, 0, SEEK_END);
        unsigned long fsize = ftell(fp);
        rewind(fp);
        unsigned char *buf = new unsigned char[fsize];
        if (fread(buf, 1, fsize, fp) != fsize) {
          delete[] buf;
          return false;
        }
        fclose(fp);

        easyexif::EXIFInfo data;
        int code = data.parseFrom(buf, fsize);
        delete[] buf;
        if (code) {
            gapSize++;
            if (gapSize >= MAX_GAP_SIZE) {
                return false;
            }
            missingMetaData.append(index);
            addGPSValue(0,0);
            continue;
        }

        if (data.GeoLocation.Altitude == 0) {
            addGPSValue(data.GeoLocation.Latitude, data.GeoLocation.Longitude);
            gapSize = 0;
        }
        else {
            addGPSValue(data.GeoLocation.Latitude, data.GeoLocation.Longitude, data.GeoLocation.Altitude);
            gapSize = 0;
        }

    }
    //No image has exif data
    if (missingMetaData.size() == paths.size()) {
        return false;
    }
    //Interpolate missing meta data
    if (missingMetaData.size() != 0) {
        interpolateMissingData(missingMetaData);
    }
    return true;
}
