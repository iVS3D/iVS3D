#ifndef GPSREADERDJIMATRICE_H
#define GPSREADERDJIMATRICE_H

#include "gpsreader.h"
#include "metadatamanager.h"
#include "stringcontainer.h"
#include <QVariant>
#include <QFile>
#include <QRegularExpression>
#include <QPointF>

/**
 * @interface GPSReaderDJI
 *
 * @ingroup Model
 *
 * @brief Class to parse meta data from DJI Matrice drones
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class GPSReaderDJIMatrice : public GPSReader
{
public:
    GPSReaderDJIMatrice();
    ~GPSReaderDJIMatrice();

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
    QString m_name = "GPSReaderDJIMatrice";
    void print(QList<QHash<QString, QVariant>> a, QString path);
};

REGISTER_METAREADER("GPSReaderDJIMatrice", GPSReaderDJIMatrice)

#endif // GPSREADERDJIMATRICE_H
