#include "metadatamanager.h"
#include "gpsreaderdji.h"


MetaDataManager::MetaDataManager()
{

}

void MetaDataManager::initMetaData(QStringList paths, Reader* images)
{
    int i = MetaDataManager::instance().m_availablerReader.size();
    for (QString path : paths) {
        for (std::pair<std::string, AbstractBuilder> a : MetaDataManager::instance().m_availablerReader) {
            MetaDataReader* current = a.second();
            if (current->parseData(path, images) == true) {
                m_parsedMetaReader.append(current);
            }
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

bool MetaDataManager::reg(std::string name, AbstractBuilder builder)
{
    return m_availablerReader.insert(std::make_pair(name,builder)).second;
}


