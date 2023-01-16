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
    for (std::string path : paths) {
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
          return false;
        }

        if (data.GeoLocation.Altitude == 0) {
            addGPSValue(data.GeoLocation.Latitude, data.GeoLocation.Longitude);
        }
        else {
           addGPSValue(data.GeoLocation.Latitude, data.GeoLocation.Longitude, data.GeoLocation.Altitude);
        }

    }
    return true;
}
