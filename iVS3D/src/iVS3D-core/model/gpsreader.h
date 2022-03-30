#ifndef GPSREADER_H
#define GPSREADER_H

#include "metadatareader.h"
#include <cmath>
#include <QPointF>
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
     * @brief GPSReader::normaliseGPS This method will interpolate gps values for every image
     * @param GPSvalues GPSValues parsed from MetaData file
     * @param timeItervall time in seconds which pass between two gps values
     * @param fps fps of the video
     * @param imageNumber number of images in the video
     * @return QList with a gps value for every image
     */
   QList<QPointF> normaliseGPS(QList<QPointF> GPSvalues, double timeItervall, double fps, uint imageNumber);

protected:
   QPointF interpolate(QPointF a, QPointF b, double t, double stepsize);
};
#endif // GPSREADER_H
