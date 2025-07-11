#include "transformmanager.h"
#include <iostream>

TransformManager &TransformManager::instance()
{
    static TransformManager INSTANCE;
    return INSTANCE;
}


TransformManager::TransformManager()
{
    loadPlugins();
    m_transformThread = new QThread;
    //Don't create widget in noUI mode
    if (!qApp->property(stringContainer::UIIdentifier).toBool()) {
        return;
    }
    for(auto& p : m_transformList){
        QWidget *w = p.transform->getSettingsWidget(nullptr);
        w->setVisible(false);
        p.transform->moveToThread(m_transformThread);
    }
    m_transformThread->start();
}

QWidget *TransformManager::getSettingsWidget(QWidget *parent, uint idx)
{
    return m_transformList[idx].transform->getSettingsWidget(parent);
}

void TransformManager::exit()
{
    std::cout << "[INFO] Cleaning up transform plugins...";
    if (m_transformThread) {
        m_transformThread->quit();
        m_transformThread->wait();
        delete m_transformThread;
        m_transformThread = nullptr;
    }

    for (auto& t : m_transformList) {
        t.transform = nullptr;
        t.loader->unload();
        delete t.loader; // Clean up the QPluginLoader
    }
    m_transformList.clear();
    std::cout << " done." << std::endl;
}

QStringList TransformManager::getTransformList()
{
    QStringList plugins;
    for (auto algorithm : m_transformList){
        plugins.append(algorithm.transform->getName());
    }
    return plugins;
}

int TransformManager::getTransformCount()
{
    return (int)m_transformList.size();
}

ITransformRequestDequeue *TransformManager::getTransform(uint id)
{
    if(id>=m_transformList.size()){
        return nullptr;
    }
    return m_transformList.at(id).transform;
}

void TransformManager::selectTransform(uint id)
{
    if(id == UINT_MAX){
        return;
    }
    m_transformList[id].transform->moveToThread(m_transformThread);
    emit sig_selectedTransformChanged(id);
}

void TransformManager::setTransformationEnabled(bool enabled)
{
    m_transformationEnabled = enabled;
    emit sig_transformEnabledChanged(enabled);
}

bool TransformManager::isTransformEnabled()
{
    return m_transformationEnabled;
}

void TransformManager::enableCuda(bool enabled)
{
    for(auto t : m_transformList){
        QMetaObject::invokeMethod(t.transform, "slot_enableCuda", Qt::DirectConnection, Q_ARG(bool, enabled));
    }
}

void TransformManager::setSettings(QMap<QString, QVariant> settings, uint idx)
{
    m_transformList[idx].transform->setSettings(settings);
}

QMap<QString, QVariant> TransformManager::getSettings(uint idx)
{
    return m_transformList[idx].transform->getSettings();
}


void TransformManager::loadPlugins(){
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
        std::cerr << "[ERROR] No plugins directory found at: " << pluginsDir.absolutePath().toStdString() << std::endl;
        return;
    }
    std::cout << "[INFO] Loading transform plugins from: " << pluginsDir.absolutePath().toStdString() << std::endl;
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader *pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader->instance();
        if(plugin){
            ITransform* algorithm = qobject_cast<ITransform*>(plugin);
            if(algorithm){
                m_transformList.push_back({ new ITransformRequestDequeue(algorithm), pluginLoader });
                std::cout << "[INFO] Loaded transform plugin: " << algorithm->getName().toStdString() << std::endl;
            } else {
                pluginLoader->unload();
                delete pluginLoader;
            }
        }
    }
}
