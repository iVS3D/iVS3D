#ifndef METADATAMANAGER_H
#define METADATAMANAGER_H

#include "metadata.h"
#include "model/metaData/gpsreader.h"
#include <QObject>
#include <functional>


typedef std::function<MetaDataReader *()> AbstractBuilder;

/**
 * @interface MetaData
 *
 * @ingroup Model
 *
 * @brief MetaDataManager loads all availble meta data Reader and lets them parse given meta data files
 *
 * @author Daniel Brommer
 *
 * @date 2022/01/09
 */

class MetaDataManager : public MetaData
{
public:

    static MetaDataManager &instance(){
        static MetaDataManager INSTANCE;
        return INSTANCE;
    }

    static const QStringList supportedFileExtensions(){
        return {".srt", ".SRT", ".gpx", ".GPX"};
    }

    /**
     * @brief initMetaData tries to load all known MetaDataReader for videos with the meta data given
     * @param paths Paths to the selected meta data files
     * @param images Reader with the currently loaded images
     */
    void initMetaDataVideo(QStringList paths, uint picCount, double fps);

    /**
     * @brief initMetaData tries to load all known MetaDataReader with the meta data given by the paths
     * @param images Reader with the currently loaded images
     */
    void initMetaDataImages(std::vector<std::string> fileVector);

    /**
     * @brief availableMetaData returns a List of the names of all MetaDataReader whichs have loaded meta data
     * @return QStringList with names of available metaDataReader
     */
    QStringList availableMetaData() override;

    /**
     * @brief loadAllMetaData returns all loaded metaDataReader
     * @return QList with all loaded metaDataReader
     */
    QList<MetaDataReader*>loadAllMetaData() override;

    /**
     * @brief loadMetaData returns the speficied metaDataReader
     * @param name Name of the metaDataReader to return
     * @return The speficied metaDataReader, nullprt if theres no metaDataReader with the given name
     */
    MetaDataReader* loadMetaData(QString name) override;

    /**
     * @brief getPaths returns the paths to all parsed metaDataFiles
     * @return Paths to all parsed metaDataFiles
     */
    QStringList getPaths() override;

    void resetData();

    bool reg(std::string name, AbstractBuilder builder);

    /**
     * @brief interpolateMissingMetaData is called when the option to interpoalte missing meta data is changed
     * @param interpolate @a true if missing meta data is interpolated, @a false otherwise
     */
    void interpolateMissingMetaData(bool interpolate);

private:
    QStringList m_uniquePaths;
    QList<MetaDataReader*> m_parsedMetaReader;
    std::map<std::string, AbstractBuilder> m_availablerReader;
    MetaDataManager();
    bool m_interpolateMissingMetaData;

};

template<typename Implementation>
MetaDataReader *builder(){
    return new Implementation();
}

#define REGISTER_METAREADER(name, impl) const bool res = MetaDataManager::instance().reg(name, builder<impl>);

#endif // METADATAMANAGER_H
