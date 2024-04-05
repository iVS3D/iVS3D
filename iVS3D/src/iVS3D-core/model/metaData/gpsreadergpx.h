#ifndef GPSREADERGPX_H
#define GPSREADERGPX_H

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

class GPSReaderGPX : public GPSReader
{
public:
    GPSReaderGPX();
    ~GPSReaderGPX();

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
    bool parseLine(QString line, QString nextLine);
    QString m_name = "GPSReaderGPX";
    void print(QList<QHash<QString, QVariant>> a, QString path);
};

REGISTER_METAREADER("GPSReaderGPX", GPSReaderGPX)

#endif // GPSREADERGPX_H
