#include "gpsreader.h"



QList<QPointF> GPSReader::normaliseGPS(QList<QPointF> GPSvalues, double timeItervall, double fps, uint imageNumber)
{

    QList<QPointF> interpolatedGPS;
    //Caculate the timestamp of every frame
    QList<double> indexToTime;
    for (int i = 0; i < imageNumber; i++) {
        indexToTime.append((double)i/fps);
    }
    for (double t : indexToTime) {
        //Get index of gps values between which the new value will be interpolated
        //If for example timeIntervall = 1 and the timestamp of the frame is 5.78, its gps value needs to interpolated with the gps values 5 and 6
        int aValue = std::floor(t / timeItervall);
        //Calculate how far the timestamp is away from the first gps value -> for above value we need to interpolate with t=0.78 betwenn value 5 and 6
        double deltaT = (t / timeItervall) - aValue;
        //do the actuall interpolation
        interpolatedGPS.append(interpolate(GPSvalues[aValue], GPSvalues[aValue+1], deltaT, timeItervall));
    }

    return interpolatedGPS;
}


QPointF GPSReader::interpolate(QPointF a, QPointF b, double t, double stepsize) {
    //linear interpolation between values a and b
    Q_ASSERT(t <= stepsize);
    double latitude = ((stepsize-t) * a.x()) + (t * b.x());
    double longitude = ((stepsize-t) * a.y()) + (t * b.y());
    //round to 5 decimal places
    latitude = roundf(latitude * 100000) / 100000;
    longitude = roundf(longitude * 100000) / 100000;
    return QPointF(latitude, longitude);
}
