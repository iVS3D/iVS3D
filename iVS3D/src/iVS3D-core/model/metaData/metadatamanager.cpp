#include "metadatamanager.h"
#include <QJsonObject>
#include <QJsonArray>

MetaDataManager::MetaDataManager()
{

}

void MetaDataManager::initMetaDataVideo(QStringList paths, uint picCount, double fps)
{
    // in case of a video, metadata might be provided in separate files, check for each of them if we support that format
    for (QString path : paths) {
        bool pathContainsValidMetaData = false;
        for (std::pair<std::string, AbstractBuilder> a : MetaDataManager::instance().m_availablerReader) {
            MetaDataReader* current = a.second();
            if (current->parseDataVideo(path, picCount, fps, m_interpolateMissingMetaData) == true) {
                if(!m_parsedMetaReader.contains(current)) m_parsedMetaReader.append(current);
                pathContainsValidMetaData = true;
            }
        }
        // keep track of all available meta data paths
        if (pathContainsValidMetaData && !m_metaDataFiles.contains(path)) m_metaDataFiles.push_back(path);
    }
}

void MetaDataManager::initMetaDataImages(std::vector<std::string> fileVector)
{
    // in case of images, each image file contais it's own metadata, i.e. exif
    for (std::pair<std::string, AbstractBuilder> a : MetaDataManager::instance().m_availablerReader) {
        MetaDataReader* current = a.second();
        bool result = current->parseDataImage(fileVector, m_interpolateMissingMetaData);
        if (result == true) {
            m_parsedMetaReader.append(current);
        }
    }
}

QStringList MetaDataManager::availableMetaData()
{
    QStringList names;
    for (MetaDataReader* meta : m_parsedMetaReader) {
        names.append(meta->getName());
    }
    return names;
}

QList<MetaDataReader *> MetaDataManager::loadAllMetaData()
{
    return m_parsedMetaReader;
}

MetaDataReader *MetaDataManager::loadMetaData(QString name)
{
    for (MetaDataReader* meta : m_parsedMetaReader) {
        if (meta->getName().compare(name) == 0) {
            return meta;
        }
    }
    return nullptr;
}

QStringList MetaDataManager::getPaths()
{
    return m_metaDataFiles;
}

void MetaDataManager::resetData()
{
    m_metaDataFiles.clear();
    for (MetaDataReader* m : m_parsedMetaReader) {
        delete m;
    }
    m_parsedMetaReader.clear();
}

bool MetaDataManager::reg(std::string name, AbstractBuilder builder)
{
    return m_availablerReader.insert(std::make_pair(name,builder)).second;
}

void MetaDataManager::interpolateMissingMetaData(bool interpolate)
{
    m_interpolateMissingMetaData = interpolate;
}

