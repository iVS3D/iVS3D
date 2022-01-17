#ifndef GPSREADERDJI_H
#define GPSREADERDJI_H

#include "gpsreader.h"
#include "metadatamanager.h"
#include "reader.h"
#include <QVariant>
#include <QFile>
#include <QRegularExpression>
#include <QPointF>

/**
 * @interface GPSReaderDJI
 *
 * @ingroup Model
 *
 * @brief Class to parse meta data from DJI drones
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class GPSReaderDJI : public GPSReader
{
public:
    GPSReaderDJI();
    ~GPSReaderDJI();

    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    QString getName();
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
    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseData(QString path, int picCount, double fps);


private:
    bool parseLine(QString line);
    QString m_name = "GPSReaderDJI";
    QString m_path;
    QList<QPointF> m_GPSvalues;
    QList<QPointF> m_GPSvaluesInterpolated;

};

REGISTER_I("GPSReaderDJI", GPSReaderDJI)

#endif // GPSREADERDJI_H
