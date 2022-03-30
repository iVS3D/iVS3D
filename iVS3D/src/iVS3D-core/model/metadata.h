#ifndef METADATA_H
#define METADATA_H

#include <QObject>
#include "metadatareader.h"

/**
 * @interface MetaData
 *
 * @ingroup Model
 *
 * @brief Interface to give plugins access to all parsed and loaded meta data
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class MetaData
{
public:
    /**
     * @brief availableMetaData returns a List of the names of all MetaDataReader whichs have loaded meta data
     * @return QStringList with names of available metaDataReader
     */
    virtual QStringList availableMetaData() = 0;

    /**
     * @brief loadAllMetaData returns all loaded metaDataReader
     * @return QList with all loaded metaDataReader
     */
    virtual QList<MetaDataReader*>loadAllMetaData() = 0;

    /**
     * @brief loadMetaData returns the speficied metaDataReader
     * @param name Name of the metaDataReader to return
     * @return The speficied metaDataReader
     */
    virtual MetaDataReader* loadMetaData(QString name) = 0;

    /**
     * @brief getPaths returns the paths to all parsed metaDataFiles
     * @return Paths to all parsed metaDataFiles
     */
    virtual QStringList getPaths() = 0;

};

#endif // METADATA_H
