#include "gpsreaderexif.h"
#include <QDebug>

GPSReaderExif::GPSReaderExif()
{

}

GPSReaderExif::~GPSReaderExif()
{
    m_GPSvalues.clear();
}

QString GPSReaderExif::getName()
{
    return m_name;
}

QVariant GPSReaderExif::getImageMetaData(uint index)
{
    return m_GPSvalues[index];
}

QList<QVariant> GPSReaderExif::getAllMetaData()
{
    QList<QVariant> values;
    for (int i = 0; i < m_GPSvalues.size(); i++) {
        values.append(m_GPSvalues[i]);
    }
    return values;
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

        QPointF gps = QPointF(data.GeoLocation.Latitude, data.GeoLocation.Longitude);
        if (gps.x() == 0 && gps.y() == 0) {
            return false;
        }
        m_GPSvalues.append(gps);
    }
    return true;
}
