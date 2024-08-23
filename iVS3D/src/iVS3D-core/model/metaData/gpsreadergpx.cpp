#include "gpsreadergpx.h"
#include "qdebug.h"

GPSReaderGPX::GPSReaderGPX() {}

GPSReaderGPX::~GPSReaderGPX()
{
    m_GPSHashs.clear();
}

QString GPSReaderGPX::getName()
{
    return m_name;
}

bool GPSReaderGPX::parseDataVideo(QString path, int picCount, double fps, bool interpolate)
{
    //fps == -1 -> Input are images
    if (fps == -1) {
        return false;
    }
    m_GPSHashs.clear();
    QFile metaFile(path);

    if (!metaFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    int matchStop = 0;
    while (!metaFile.atEnd()) {
        QRegularExpression matchXML("<trkpt");
        QByteArray line = metaFile.readLine();
        if (matchXML.match(line).hasMatch() == true) {
            if (!parseLine(line, metaFile.readLine())) {
                return false;
            }
            matchStop = 0;
        }
        //Stop parsing if no correct line has been found 3 times
        else {
            if (matchStop < 9) {
                matchStop++;
            }
            else return false;
        }

    }
    return normaliseGPS(picCount);

}

bool GPSReaderGPX::parseLine(QString line, QString nextLine)
{
    QRegularExpression matchAltitudeGPS("<ele>(-?\\d{1,5}.\\d{1,5})</ele>");
    QRegularExpressionMatch matchAltitude = matchAltitudeGPS.match(nextLine);
    double altitude;
    double latitude;
    double longitude;
    bool useAltitude = false;
    if (matchAltitude.hasMatch()) {
        useAltitude = true;
        altitude = matchAltitude.captured(1).toDouble();
    }
    QRegularExpression matchLatitudeGPS("lat=\"(-?\\d{1,10}.\\d{1,10})\"");
    QRegularExpressionMatch matchLatitude = matchLatitudeGPS.match(line);
    QRegularExpression matchLongtitudeGPS("lon=\"(-?\\d{1,10}.\\d{1,10})\"");
    QRegularExpressionMatch matchLongtitude = matchLongtitudeGPS.match(line);

    if (matchLatitude.hasMatch() && matchLongtitude.hasMatch()) {
        latitude = matchLatitude.captured(1).toDouble();
        longitude = matchLongtitude.captured(1).toDouble();

        if (useAltitude) {
            if (!addGPSValue(latitude, longitude, altitude)) {
                return false;
            }
        }
        else {
            if (!addGPSValue(latitude, longitude)) {
                return false;
            }
        }
        return true;
    }
    else return false;


}

void GPSReaderGPX::print(QList<QHash<QString, QVariant>> a, QString path)
{
    QString line;
    for(QHash<QString, QVariant> values : a) {
        double latitudeA = values.find(stringContainer::latitudeIdentifier).value().toDouble();
        double longitudeA = values.find(stringContainer::longitudeIdentifier).value().toDouble();
        double altitudeA = values.find(stringContainer::altitudeIdentifier).value().toDouble();
        line.append(QString::number(latitudeA));
        line.append(",");
        line.append(QString::number(longitudeA));
        line.append(",");
        line.append(QString::number(altitudeA));
        line.append("\n");
    }
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
      QTextStream out(&file); out << line;
      file.close();
    }
}
