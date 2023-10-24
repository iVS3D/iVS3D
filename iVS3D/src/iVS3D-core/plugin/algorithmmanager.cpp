#include "algorithmmanager.h"



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

QStringList AlgorithmManager::getAlgorithmNames()
{
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

QString AlgorithmManager::getPluginFileNameToIndex(int index)
{
    return m_pluginFileNames[index];
}

QString AlgorithmManager::getPluginNameFromFileName(QString FileName)
{
    // Export doesnt have a plugin file for translation
    if (FileName == "Export") {
        return FileName;
    }

    int index = m_pluginFileNames.indexOf(FileName);
    return m_algorithmList.at(index)->getName();
}

int AlgorithmManager::getIndexFromFileName(QString FileName)
{
    return m_pluginFileNames.indexOf(FileName);
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




void AlgorithmManager::notifyNewMetaData()
{
    m_sigObj->newMetaData();
}

void AlgorithmManager::notifySelectedImageIndex(uint index)
{
    m_sigObj->selectedImageIndex(index);
}

void AlgorithmManager::notifyKeyframesChanged(std::vector<uint> keyframes)
{
    m_sigObj->keyframesChanged(keyframes);
}

void AlgorithmManager::slot_updateKeyframes(std::vector<uint> keyframes)
{
    QObject* sender = QObject::sender();
    IAlgorithm* algorithm = qobject_cast<IAlgorithm*>(sender);
    if (algorithm) {
        QString currentName = algorithm->getName();
        emit sig_updateKeyframe(currentName, keyframes);
    }
}

void AlgorithmManager::slot_updateBuffer(QMap<QString, QVariant> buffer)
{
    QObject* sender = QObject::sender();
    IAlgorithm* algorithm = qobject_cast<IAlgorithm*>(sender);
    if (algorithm) {
        QString currentName = algorithm->getName();
        emit sig_updateBuffer(currentName, buffer);
    }
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
        if(fileName == "libiVS3D-pluginInterface.so")
            continue;

        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader.instance();
        if(plugin){

            IAlgorithm* algorithm = qobject_cast<IAlgorithm*>(plugin);
            if(algorithm){
                m_algorithmList.push_back(algorithm);
                connect(algorithm, &IAlgorithm::updateKeyframes, this, &AlgorithmManager::slot_updateKeyframes);
                connect(algorithm, &IAlgorithm::updateBuffer, this, &AlgorithmManager::slot_updateBuffer);
                m_pluginFileNames.append(QFileInfo(fileName).baseName());
            } else {
                pluginLoader.unload();
            }
        }
        else
        {
            qWarning() << "[WARN]" << pluginLoader.errorString();
        }
    }
}
