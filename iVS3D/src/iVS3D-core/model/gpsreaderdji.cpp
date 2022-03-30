#include "gpsreaderdji.h"



GPSReaderDJI::GPSReaderDJI()
{

}

GPSReaderDJI::~GPSReaderDJI()
{
    m_GPSvalues.clear();
    m_GPSvaluesInterpolated.clear();
}

QString GPSReaderDJI::getName()
{
    return m_name;
}

QVariant GPSReaderDJI::getImageMetaData(uint index)
{
    return m_GPSvaluesInterpolated[index];
}

QList<QVariant> GPSReaderDJI::getAllMetaData()
{
    QList<QVariant> values;
    for (int i = 0; i < m_GPSvaluesInterpolated.size(); i++) {
        values.append(m_GPSvaluesInterpolated[i]);
    }
    return values;
}

bool GPSReaderDJI::parseDataVideo(QString path, int picCount, double fps)
{
    //fps == -1 -> Input are images
    if (fps == -1) {
        return false;
    }
    m_GPSvalues.clear();
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
    m_GPSvaluesInterpolated = normaliseGPS(m_GPSvalues, 1, fps, picCount);
    return m_GPSvaluesInterpolated.size() == picCount;
}


bool GPSReaderDJI::parseLine(QString line)
{
    QRegularExpression matchGPS("(\\d{0,2}.\\d{4}),(\\d{0,2}.\\d{4})");
    QRegularExpressionMatch match = matchGPS.match(line);
    if (match.hasMatch()) {
        double latitude = match.captured(1).toDouble();
        double longitude = match.captured(2).toDouble();
        m_GPSvalues.append(QPointF(latitude, longitude));
        return true;
    }
    else return false;


}
