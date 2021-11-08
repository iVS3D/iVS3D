#include "modelalgorithm.h"


ModelAlgorithm::ModelAlgorithm()
{


}

void ModelAlgorithm::addPluginBuffer(QString pluginName, QString bufferName, QVariant value)
{
    QMap<QString, QVariant> innerMap;
    if (m_pluginBuffer.contains(pluginName)) {
        innerMap = m_pluginBuffer.value(pluginName);
        innerMap.insert(bufferName, value);
        m_pluginBuffer.insert(pluginName, innerMap);
        return;
    }

    innerMap.insert(bufferName, value);
    m_pluginBuffer.insert(pluginName, innerMap);
    return;
}

QMap<QString, QMap<QString, QVariant>> ModelAlgorithm::getPluginBuffer()
{
    return m_pluginBuffer;
}


QVariant ModelAlgorithm::toText()
{
    QJsonObject jsonObject;
    QMapIterator<QString, QMap<QString, QVariant>> mapIt(m_pluginBuffer);
    while (mapIt.hasNext()) {
        mapIt.next();
        QString pluginName = mapIt.key();
        QVariant buffer(mapIt.value());
        jsonObject.insert(pluginName, QJsonValue::fromVariant(buffer));
    }
    return QVariant(jsonObject);

}

void ModelAlgorithm::fromText(QVariant data)
{
    m_pluginBuffer.clear();
    QMap<QString, QVariant> bufferMap = data.toMap();
    QMapIterator<QString, QVariant> mapIt(bufferMap);
    while (mapIt.hasNext()) {
        mapIt.next();
        QMap<QString, QVariant> innerMap = mapIt.value().toMap();
        m_pluginBuffer.insert(mapIt.key(), innerMap);
    }


}



