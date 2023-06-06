#ifndef GPSREADERDJIMAVIC_H
#define GPSREADERDJIMAVIC_H

#include "gpsreader.h"
#include "metadatamanager.h"
#include "stringcontainer.h"
#include <QVariant>
#include <QFile>
#include <QRegularExpression>
#include <QPointF>

/**
 * @interface GPSReaderDJI2
 *
 * @ingroup Model
 *
 * @brief Class to parse meta data from DJI Mavic drones
 *
 * @author Max Hermann
 *
 * @date 2023/06/06
 */

class GPSReaderDJIMavic : public GPSReader
{
public:
    GPSReaderDJIMavic();
    ~GPSReaderDJIMavic();

    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    QString getName();

    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseDataVideo(QString path, int picCount, double fps);


private:
    bool parseLine(QString line);
    QString m_name = "GPSReaderDJIMavic";
    void print(QList<QHash<QString, QVariant>> a, QString path);
};

REGISTER_METAREADER("GPSReaderDJIMavic", GPSReaderDJIMavic)

#endif // GPSREADERDJIMAVIC_H
