#include "gpsreader.h"


bool GPSReader::normaliseGPS(uint imageNumber)
{
    int exisitingValues = m_GPSHashs.size();
    bool hasAltitude = hasAltitudeData();
    //No interpolation is needed --> number of GPS points is equal or slightly higher
    if (exisitingValues == imageNumber || (exisitingValues > imageNumber && exisitingValues < imageNumber * 1.05)) {
        return true;
    }

    // TODO implement: way more GPS then images
    if (exisitingValues > imageNumber){
        return false;
    }

    QList<QHash<QString, QVariant>> GPSHashsOld = m_GPSHashs;
    m_GPSHashs.clear();

    for (int i = 0; i < imageNumber; i++) {

        double step = double(exisitingValues - 1) / imageNumber * i;
        int aValue = int(step);
        double deltaT = step - aValue;
        QGeoCoordinate geo = interpolateSingle(GPSHashsOld.at(aValue), GPSHashsOld.at(aValue + 1), deltaT, 1.0);
        if (hasAltitude) {
            addGPSValue(geo.latitude(), geo.longitude(), geo.altitude());
        }
        else {
            addGPSValue(geo.latitude(), geo.longitude());
        }
    }
    return true;
}

QVariant GPSReader::getImageMetaData(uint index)
{
    if (m_altitudeDiff != 0) {
        addAltitudeDiff(index);
    }
    return m_GPSHashs[index];
}

QList<QVariant> GPSReader::getAllMetaData()
{
    QList<QVariant> values;
    for (int i = 0; i < m_GPSHashs.size(); i++) {
        if (m_altitudeDiff != 0) {
            addAltitudeDiff(i);
        }
        values.append(m_GPSHashs[i]);
    }
    return values;
}

void GPSReader::setAltitudeDiff(double setAltitude)
{
    if (!hasAltitudeData()) {
        return;
    }
    QHash<QString, QVariant> gpsHash = m_GPSHashs.at(0);
    double altitude_abs = gpsHash.find("GPSAltitude").value().toDouble();
    double altitude = (gpsHash.find("GPSAltitudeRef").value().toString() == "0") ? altitude_abs : altitude_abs * -1;
    m_altitudeDiff = setAltitude - altitude;
}


bool GPSReader::addGPSValue(double latitude, double longitude)
{
    //check for valid lat/long values (-90 to 90 for latitude and -180 to 180 for longitude)
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
        QHash<QString, QVariant> newGPSHash = createGPSHash(0, 0);
        m_GPSHashs.push_back(newGPSHash);
        return false;
    }
    QHash<QString, QVariant> newGPSHash = createGPSHash(latitude, longitude);
    m_GPSHashs.push_back(newGPSHash);
    return true;
}

bool GPSReader::addGPSValue(double latitude, double longitude, double altitude)
{
    //check for valid lat/long values (-90 to 90 for latitude and -180 to 180 for longitude)
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
        QHash<QString, QVariant> newGPSHash = createGPSHash(0, 0);
        m_GPSHashs.push_back(newGPSHash);
        return false;
    }
    QHash<QString, QVariant> newGPSHash = createGPSHash(latitude, longitude, altitude);
    m_GPSHashs.push_back(newGPSHash);
    return true;
}

void GPSReader::addAltitudeDiff(int index)
{
   QHash<QString, QVariant> GPSHash = m_GPSHashs.at(index);
   QHash<QString, QVariant>::iterator altitudeIter = GPSHash.find(stringContainer::altitudeIdentifier);
   double oldAltitude = altitudeIter.value().toDouble();
   GPSHash.erase(altitudeIter);
   GPSHash.erase(GPSHash.find(stringContainer::altitudeIdentifier));
   double altitude = m_altitudeDiff + oldAltitude;
   QString altRef = (altitude > 0) ? stringContainer::altitudeAboveSea : stringContainer::altitudeBelowSea;
   GPSHash.insert(stringContainer::altitudeIdentifier, QVariant(abs(altitude)));
   GPSHash.insert(stringContainer::altitudeRefIdentifier, QVariant(altRef));
   m_GPSHashs.replace(index, GPSHash);
}

void GPSReader::interpolateMissingData(QList<int> missingMetaData)
{
    QList<int> continousMissing = QList<int>{missingMetaData.at(0)};
    for (int index = 0; index < missingMetaData.size(); index++) {
        //find continous gaps in the meta data
        if (index + 1 < missingMetaData.size() && missingMetaData.at(index) + 1 == missingMetaData.at(index + 1)) {
            continousMissing.append(missingMetaData.at(index + 1));
        }
        else {
            //first frame(s) do not have meta data
            if (continousMissing.first() == 0) {
                int indexOfFirstData = continousMissing.last() + 1;
                QHash<QString, QVariant> firstData = m_GPSHashs.at(indexOfFirstData);
                for (int gapIndex = 0; gapIndex < indexOfFirstData; gapIndex++) {
                    m_GPSHashs.replace(continousMissing.at(gapIndex), firstData);
                }
            }
            //last frame(s) do not have meta data
            else if (continousMissing.last() == m_GPSHashs.size() - 1) {
                int indexOfLastData = continousMissing.first() - 1;
                QHash<QString, QVariant> firstData = m_GPSHashs.at(indexOfLastData);
                for (int gapIndex = 0; gapIndex < indexOfLastData; gapIndex++) {
                    m_GPSHashs.replace(continousMissing.at(gapIndex), firstData);
                }
            }
            //interpolate missing frames
            else {
                int gapStart = continousMissing.first() - 1;
                int gapEnd = continousMissing.last() + 1;
                int gapSize = continousMissing.size();
                //Get values and their ref
                QHash<QString, QVariant> a = m_GPSHashs.at(gapStart);
                QHash<QString, QVariant> b = m_GPSHashs.at(gapEnd);
                for (int gapIndex = 0; gapIndex < gapSize; gapIndex++) {
                    double t = (1.0 / (double) (gapSize + 1)) * (gapIndex + 1);
                    QGeoCoordinate newGeo = interpolateSingle(a, b, t, 1);
                    QHash<QString, QVariant> newGPSHash = createGPSHash(newGeo.latitude(), newGeo.longitude(), newGeo.altitude());
                    m_GPSHashs.replace(continousMissing.at(gapIndex), newGPSHash);
                }
            }
            continousMissing.clear();
            if (index + 1 < missingMetaData.size()) {
                continousMissing.append(missingMetaData.at(index + 1));
            }

        }

    }
}

QGeoCoordinate GPSReader::interpolateSingle(QHash<QString, QVariant> a, QHash<QString, QVariant> b, double t, double stepsize)
{
    //linear interpolation between values a and b
    Q_ASSERT(t <= stepsize);
    //Get values and their ref
    bool useAltitude = a.find(stringContainer::altitudeIdentifier) != a.end();
    double latitudeA_abs = a.find(stringContainer::latitudeIdentifier).value().toDouble();
    double latitudeA = (a.find(stringContainer::latitudeRefIdentifier).value().toString() == "N") ? latitudeA_abs : latitudeA_abs * -1;
    double longitudeA_abs = a.find(stringContainer::longitudeIdentifier).value().toDouble();
    double longitudeA = (a.find(stringContainer::longitudeRefIdentifier).value().toString() == "E") ? longitudeA_abs : longitudeA_abs * -1;

    double latitudeB_abs = b.find(stringContainer::latitudeIdentifier).value().toDouble();
    double latitudeB = (b.find(stringContainer::latitudeRefIdentifier).value().toString() == "N") ? latitudeB_abs : latitudeB_abs * -1;
    double longitudeB_abs = b.find(stringContainer::longitudeIdentifier).value().toDouble();
    double longitudeB = (b.find(stringContainer::longitudeRefIdentifier).value().toString() == "E") ? longitudeB_abs : longitudeB_abs * -1;

    //interpolate lat,long and alt according to stepsize
    double latitude = ((stepsize-t) * latitudeA) + (t * latitudeB);
    double longitude = ((stepsize-t) * longitudeA) + (t * longitudeB);

    //round to 5 decimal places
    latitude = roundf(latitude * 100000) / 100000;
    longitude = roundf(longitude * 100000) / 100000;

    if (useAltitude) {
        double altitudeA_abs = a.find(stringContainer::altitudeIdentifier).value().toDouble();
        double altitudeA = (a.find(stringContainer::altitudeRefIdentifier).value().toString() == "0") ? altitudeA_abs : altitudeA_abs * -1;

        double altitudeB_abs = b.find(stringContainer::altitudeIdentifier).value().toDouble();
        double altitudeB = (b.find(stringContainer::altitudeRefIdentifier).value().toString() == "0") ? altitudeB_abs : altitudeB_abs * -1;

        double altitude = ((stepsize-t) * altitudeA) + (t * altitudeB);

        altitude = roundf(altitude * 100000) / 100000;
        QGeoCoordinate geo(latitude, longitude, altitude);
        return geo;
    }
    else {
        QGeoCoordinate geo(latitude, longitude);
        return geo;
    }
}

QHash<QString, QVariant> GPSReader::createGPSHash(double latitude, double longitude, double altitude)
{
    QHash<QString, QVariant> newGPSHash;
    QString latRef = (latitude > 0) ? stringContainer::latitudeNorth : stringContainer::latitudeSouth;
    QString longRef = (longitude > 0) ? stringContainer::longitudeEast : stringContainer::longitudeWest;

    //abs to safe positive values. latRef and longRef show orientation
    newGPSHash.insert(stringContainer::latitudeIdentifier, QVariant(abs(latitude)));
    newGPSHash.insert(stringContainer::longitudeIdentifier, QVariant(abs(longitude)));
    newGPSHash.insert(stringContainer::latitudeRefIdentifier, QVariant(latRef));
    newGPSHash.insert(stringContainer::longitudeRefIdentifier, QVariant(longRef));

    if (altitude != DBL_MAX) {
        QString altRef = (altitude > 0) ? stringContainer::altitudeAboveSea : stringContainer::altitudeBelowSea;
        newGPSHash.insert(stringContainer::altitudeIdentifier, QVariant(abs(altitude)));
        newGPSHash.insert(stringContainer::altitudeRefIdentifier, QVariant(altRef));
    }
    return newGPSHash;

}

bool GPSReader::hasAltitudeData()
{
    QHash<QString, QVariant> GPSHash = m_GPSHashs.at(0);
    QHash<QString, QVariant>::iterator altitudeIter = GPSHash.find(stringContainer::altitudeIdentifier);
    return (altitudeIter == GPSHash.end() ? false : true);
}
