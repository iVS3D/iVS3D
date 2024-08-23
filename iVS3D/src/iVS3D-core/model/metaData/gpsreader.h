#ifndef GPSREADER_H
#define GPSREADER_H

#include "metadatareader.h"
#include "stringcontainer.h"
#include <cmath>
#include <cfloat>
#include <QPointF>
#include <QHash>
#include <QVariant>
#include <QGeoCoordinate>

/**
 * @interface GPSReader
 *
 * @ingroup Model
 *
 * @brief Interface for classes wich loaded meta files containing GPS data
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class GPSReader : public MetaDataReader
{
public:
    /**
     * @brief GPSReader::normaliseGPS This method will interpolate gps values (latitude, longitude, attitude) for every images
     * @param imageNumber number of images in the video
     */
   bool normaliseGPS(uint imageNumber);
   /**
    * @brief getImageMetaData Returns parsed meta data from the index images
    * @param index Index of the image to get the meat data from
    * @return QVaraint containing the meta data
    */
   QVariant getImageMetaData(uint index);
   /**
    * @brief getAllMetaData Returns all meta data in the same order as the images ar
    * @return QList with the meta data from all images
    */
   QList<QVariant> getAllMetaData();

   void setAltitudeDiff(double setAltitude);

   bool hasAltitudeData();



protected:
   bool addGPSValue(double latitude, double longitude);
   bool addGPSValue(double latitude, double longitude, double altitude);
   QList<QHash<QString, QVariant>> m_GPSHashs;
   double m_altitudeDiff = 0;
   void addAltitudeDiff(int index);
   void interpolateMissingData(QList<int> missingMetaData);
   const int MAX_GAP_SIZE = 5;

private:
   QGeoCoordinate interpolateSingle(QHash<QString, QVariant> a, QHash<QString, QVariant> b, double t, double stepsize);
   QHash<QString, QVariant> createGPSHash(double latitude, double longitude, double altitude = DBL_MAX);

};
#endif // GPSREADER_H
