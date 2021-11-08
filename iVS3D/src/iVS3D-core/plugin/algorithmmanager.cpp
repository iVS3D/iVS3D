#include "algorithmmanager.h"

AlgorithmManager::AlgorithmManager()
{
    loadPlugins();
}

AlgorithmManager &AlgorithmManager::instance()
{
    static AlgorithmManager INSTANCE;
    return INSTANCE;
}

QWidget* AlgorithmManager::getSettingsWidget(QWidget* parent, uint idx){
    if (idx > m_algorithmList.size()){
        return nullptr;
    }
    QWidget* widget = m_algorithmList[idx]->getSettingsWidget(parent);
    return widget;
}

std::vector<uint> AlgorithmManager::sample(Reader* images, std::vector<uint> sharpImages, Progressable* receiver, volatile bool* stopped, int idx, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent *logFile){
    return m_algorithmList[idx]->sampleImages(images, sharpImages, receiver, stopped, buffer, useCuda, logFile);
}

QStringList AlgorithmManager::getAlgorithmList(){

    QStringList plugins;
    foreach(IAlgorithm* algorithm, m_algorithmList){
        plugins.append(algorithm->getName());
    }
    return plugins;
}


QString AlgorithmManager::getPluginNameToIndex(int index)
{
    return m_algorithmList[index]->getName();
}

QVariant AlgorithmManager::getBuffer(int idx)
{
    return m_algorithmList[idx]->getBuffer();
}

QString AlgorithmManager::getBufferName(int idx)
{
    return m_algorithmList[idx]->getBufferName();
}

int AlgorithmManager::getAlgorithmCount()
{
    return (int)m_algorithmList.size();
}

void AlgorithmManager::initializePlugins(Reader *reader)
{
    for (uint i = 0; i < m_algorithmList.size(); i++) {
        m_algorithmList[i]->initialize(reader);
    }
    return;
}

void AlgorithmManager::setSettings(int idx, QMap<QString, QVariant> settings)
{
    m_algorithmList[idx]->setSettings(settings);
}

QMap<QString, QVariant> AlgorithmManager::generateSettings(int idx, Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool* stopped)
{
    return m_algorithmList[idx]->generateSettings(receiver, buffer, useCuda, stopped);
}

QMap<QString, QVariant> AlgorithmManager::getSettings(int idx)
{
    return m_algorithmList[idx]->getSettings();
}

void AlgorithmManager::loadPlugins(){
    QDir pluginsDir(QCoreApplication::applicationDirPath());
    #if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release"){
                pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
    #elif defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
    #endif
    bool foundPlugin = pluginsDir.cd("plugins");
    if (!foundPlugin) {
        qDebug() << "No plugins found";
        return;
    }
    qDebug() << pluginsDir.absolutePath();
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader.instance();
        if(plugin){

            IAlgorithm* algorithm = qobject_cast<IAlgorithm*>(plugin);
            if(algorithm){
                qDebug() << algorithm->getName();
                m_algorithmList.push_back(algorithm);
            } else {
                pluginLoader.unload();
            }
        }
    }
    qDebug() << QString::number(m_algorithmList.size());
}
