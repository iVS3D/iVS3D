#include "algorithmmanager.h"
#include "algorithmcontroller.h"


AlgorithmManager::AlgorithmManager()
{
    m_sigObj = new signalObject();
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

std::vector<uint> AlgorithmManager::sample(std::vector<uint> sharpImages, Progressable* receiver, volatile bool* stopped, int idx, bool useCuda, LogFileParent *logFile){
    return m_algorithmList[idx]->sampleImages(sharpImages, receiver, stopped, useCuda, logFile);
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

int AlgorithmManager::getAlgorithmCount()
{
    return (int)m_algorithmList.size();
}

void AlgorithmManager::initializePlugins(Reader *reader, QMap<QString, QMap<QString, QVariant>> allBuffer)
{
    for (uint i = 0; i < m_algorithmList.size(); i++) {
        auto name = m_algorithmList[i]->getName();
        auto buf = allBuffer[name];
        m_algorithmList[i]->initialize(reader, buf, m_sigObj);
    }
    return;
}

void AlgorithmManager::setSettings(int idx, QMap<QString, QVariant> settings)
{
    m_algorithmList[idx]->setSettings(settings);
}

QMap<QString, QVariant> AlgorithmManager::generateSettings(int idx, Progressable *receiver, bool useCuda, volatile bool* stopped)
{
    return m_algorithmList[idx]->generateSettings(receiver, useCuda, stopped);
}

QMap<QString, QVariant> AlgorithmManager::getSettings(int idx)
{
    return m_algorithmList[idx]->getSettings();
}

void AlgorithmManager::connectController(AlgorithmController *controller)
{
    for(IAlgorithm* algo : m_algorithmList) {
        connect(algo, &IAlgorithm::updateKeyframes, controller, &AlgorithmController::slot_updateKeyframes);
        connect(algo, &IAlgorithm::updateBuffer, controller, &AlgorithmController::slot_updateBuffer);
    }
}

void AlgorithmManager::disconnectController(AlgorithmController *controller)
{
    for(IAlgorithm* algo : m_algorithmList) {
        disconnect(algo, &IAlgorithm::updateKeyframes, controller, &AlgorithmController::slot_updateKeyframes);
        disconnect(algo, &IAlgorithm::updateBuffer, controller, &AlgorithmController::slot_updateBuffer);
    }
}



void AlgorithmManager::sigNewMetaData()
{
    m_sigObj->newMetaData();
}

void AlgorithmManager::sigSelectedImageIndex(uint index)
{
    m_sigObj->selectedImageIndex(index);
}

void AlgorithmManager::sigKeyframesChanged(std::vector<uint> keyframes)
{
    m_sigObj->keyframesChanged(keyframes);
}

void AlgorithmManager::sigUpdateBuffer(QString pluginName, QMap<QString, QVariant> buffer)
{
    m_sigObj->updateBuffer(pluginName, buffer);
}

void AlgorithmManager::loadPlugins(){
    QDir pluginsDir(QCoreApplication::applicationDirPath());
    #if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release"){
                pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
    #elif defined(Q_OS_LINUX)
        if (pluginsDir.dirName().toLower() == "ivs3d-core"){
         pluginsDir.cdUp();
        }

    #endif
    bool foundPlugin = pluginsDir.cd("plugins");
    if (!foundPlugin) {
        return;
    }
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader.instance();
        if(plugin){

            IAlgorithm* algorithm = qobject_cast<IAlgorithm*>(plugin);
            if(algorithm){
                m_algorithmList.push_back(algorithm);
            } else {
                pluginLoader.unload();
            }
        }
    }
}
