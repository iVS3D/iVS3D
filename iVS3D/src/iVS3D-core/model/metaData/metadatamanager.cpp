#include "metadatamanager.h"


MetaDataManager::MetaDataManager()
{

}

void MetaDataManager::initMetaDataVideo(QStringList paths, uint picCount, double fps)
{
    for (QString path : paths) {
        for (std::pair<std::string, AbstractBuilder> a : MetaDataManager::instance().m_availablerReader) {
            MetaDataReader* current = a.second();
            if (current->parseDataVideo(path, picCount, fps, m_interpolateMissingMetaData) == true) {
                m_parsedMetaReader.append(current);
            }
        }
    }
}

void MetaDataManager::initMetaDataImages(std::vector<std::string> fileVector)
{
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
    return m_uniquePaths;
}

void MetaDataManager::resetData()
{
    m_uniquePaths.clear();
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


