#ifndef GPSREADER_H
#define GPSREADER_H

#include "metadatareader.h"
#include "stringcontainer.h"
#include <cmath>
#include <QPointF>
#include <QHash>
#include <QVariant>

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
     * @brief GPSReader::normaliseGPS This method will interpolate gps values (latitude, longitude, attitude) for every image
     * @param timeItervall time in seconds which pass between two gps values
     * @param fps fps of the video
     * @param imageNumber number of images in the video
     */
   void normaliseGPS(double timeItervall, double fps, uint imageNumber);
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
   void interpolate(QHash<QString, QVariant> a, QHash<QString, QVariant> b, double t, double stepsize);
   void addGPSValue(double latitude, double longitude);
   void addGPSValue(double latitude, double longitude, double altitude);
   QList<QHash<QString, QVariant>> m_GPSHashs;
   double m_altitudeDiff = 0;
   void addAltitudeDiff(int index);
};
#endif // GPSREADER_H
