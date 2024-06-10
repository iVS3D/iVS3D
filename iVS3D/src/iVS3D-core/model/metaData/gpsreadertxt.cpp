#include "gpsreadertxt.h"
#include "qdebug.h"

GPSReaderTXT::GPSReaderTXT() {}

GPSReaderTXT::~GPSReaderTXT()
{
    m_GPSHashs.clear();
}

QString GPSReaderTXT::getName()
{
    return m_name;
}

bool GPSReaderTXT::parseDataVideo(QString path, int picCount, double fps, bool interpolate)
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
        QByteArray line = metaFile.readLine();
        if (parseLine(line)) {
            matchStop = 0;
        }
        //Stop parsing if no correct line has been found 3 times
        else {
            if (matchStop < 3) {
                matchStop++;
            } else
                return false;
        }
    }
    return normaliseGPS(picCount);

}

bool GPSReaderTXT::parseLine(QString line)
{
    QStringList stringList = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);

    std::vector<float> result;
    bool sucess;
    for (QString item : stringList) {
        float itemNumber = item.toFloat(&sucess);
        if (sucess) {
            result.push_back(itemNumber);
        } else {
            return false;
        }
    }
    double latitude;
    double longitude;

    if (result.size() < 4) {
        return false;
    } else if (result.size() < 5) {
        latitude = result[2];
        longitude = result[3];
        sucess = addGPSValue(latitude, longitude);
    } else {
        latitude = result[2];
        longitude = result[3];
        double altitude = result[4];
        sucess = addGPSValue(latitude, longitude, altitude);
    }
    return sucess;
}

void GPSReaderTXT::print(QList<QHash<QString, QVariant>> a, QString path)
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
