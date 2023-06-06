#include "gpsreaderdjimatrice.h"
#include "qdebug.h"

GPSReaderDJIMatrice::GPSReaderDJIMatrice() {}

GPSReaderDJIMatrice::~GPSReaderDJIMatrice()
{
    m_GPSHashs.clear();
}

QString GPSReaderDJIMatrice::getName()
{
    return m_name;
}

bool GPSReaderDJIMatrice::parseDataVideo(QString path, int picCount, double fps)
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
        QRegularExpression matchInt("^[0-9]+$");
        QByteArray line = metaFile.readLine();
        if (matchInt.match(line).hasMatch() == true) {
            QByteArray line = metaFile.readLine();
            line = metaFile.readLine();
            line = metaFile.readLine();
            if (!parseLine(line)) {
                return false;
            }
            matchStop = 0;
        }
        //Stop parsing if no correct line has been found 3 times
        else {
            if (matchStop < 4) {
                matchStop++;
            }
            else return false;
        }

    }
    normaliseGPS(fps, picCount);
    return m_GPSHashs.size() == picCount;
}

bool GPSReaderDJIMatrice::parseLine(QString line)
{
    QHash<QString, QVariant> newGPSHash;
    QRegularExpression matchAltitudeGPS("BAROMETER:(-?\\d{1,5}.\\d{1,5})M");
    QRegularExpressionMatch matchAltitude = matchAltitudeGPS.match(line);
    double altitude;
    double latitude;
    double longitude;
    bool useAltitude = false;
    if (matchAltitude.hasMatch()) {
        useAltitude = true;
        altitude = matchAltitude.captured(1).toDouble();
    }
    QRegularExpression matchGPS("(-?\\d{0,2}.\\d{4}),(-?\\d{0,2}.\\d{4})");
    QRegularExpressionMatch match = matchGPS.match(line);
    if (match.hasMatch()) {
        latitude = match.captured(1).toDouble();
        longitude = match.captured(2).toDouble();

        if (useAltitude) {
            addGPSValue(latitude, longitude, altitude);
        }
        else {
            addGPSValue(latitude, longitude);
        }
        return true;
    }
    else return false;


}

void GPSReaderDJIMatrice::print(QList<QHash<QString, QVariant>> a, QString path)
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
