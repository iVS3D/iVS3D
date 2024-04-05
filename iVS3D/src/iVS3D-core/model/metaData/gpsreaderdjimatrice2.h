#ifndef GPSREADERDJIMATRICE2_H
#define GPSREADERDJIMATRICE2_H

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
 * @brief Class to parse meta data from DJI Matrice drones (new model)
 *
 * @author Max Hermann
 *
 * @date 2024/25/04
 */

class GPSReaderDJIMatrice2 : public GPSReader
{
public:
    GPSReaderDJIMatrice2();
    ~GPSReaderDJIMatrice2();

    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    QString getName();

    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @param interpolate indicades wether the reader is allowed to interpolate missing meta data
     * @return @a True if meta data have been loaded @a False otherwise
     */
    bool parseDataVideo(QString path, int picCount, double fps, bool interpolate);


private:
    bool parseLine(QString line);
    QString m_name = "GPSReaderDJIMatrice2";
    void print(QList<QHash<QString, QVariant>> a, QString path);
};

REGISTER_METAREADER("GPSReaderDJIMatrice2", GPSReaderDJIMatrice2)

#endif // GPSREADERDJIMATRICE2_H
