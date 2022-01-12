#ifndef METADATAREADER_H
#define METADATAREADER_H

#include <QObject>
#include "reader.h"

/**
 * @interface MetaDataReader
 *
 * @ingroup Model
 *
 * @brief Interface to load meta data
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class MetaDataReader
{
public:
    /**
     * @brief getName Returns name of meta data
     * @return Name of meta data
     */
    virtual QString getName() = 0;
    /**
     * @brief getImageMetaData Returns parsed meta data from the index images
     * @param index Index of the image to get the meat data from
     * @return QVaraint containing the meta data
     */
    virtual QVariant getImageMetaData(uint index) = 0;
    /**
     * @brief getAllMetaData Returns all meta data in the same order as the images ar
     * @return QList with the meta data from all images
     */
    virtual QList<QVariant> getAllMetaData() = 0;
    /**
     * @brief parseData Tries to load meta data from the given file
     * @param path Path to the meat data file
     * @param images Reader with the loaded images
     * @return @a True if meta data have been loaded @a False otherwise
     */
    virtual bool parseData(QString path, Reader* images) = 0;


};

#endif // METADATAREADER_H
