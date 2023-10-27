#include "transformmanager.h"
TransformManager &TransformManager::instance()
{
    static TransformManager INSTANCE;
    return INSTANCE;
}


TransformManager::TransformManager()
{
    loadPlugins();
    //Don't create widget in noUI mode
    if (!qApp->property(stringContainer::UIIdentifier).toBool()) {
        return;
    }
    for(auto *p : m_transformList){
        QWidget *w = p->getSettingsWidget(nullptr);
        w->setVisible(false);
        p->moveToThread(&m_transformThread);
    }
    m_transformThread.start();
}

QWidget *TransformManager::getSettingsWidget(QWidget *parent, uint idx)
{
    return m_transformList[idx]->getSettingsWidget(parent);
}

QStringList TransformManager::getTransformList()
{
    QStringList plugins;
    foreach(ITransform* algorithm, m_transformList){
        plugins.append(algorithm->getName());
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
    return m_transformList.at(id);
}

void TransformManager::selectTransform(uint id)
{
    if(id == UINT_MAX){
        return;
    }
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

void TransformManager::exit()
{
    m_transformThread.quit();
    m_transformThread.wait();
}

void TransformManager::enableCuda(bool enabled)
{
    for(auto *t : m_transformList){
        QMetaObject::invokeMethod(t, "slot_enableCuda", Qt::DirectConnection, Q_ARG(bool, enabled));
    }
}

void TransformManager::setSettings(QMap<QString, QVariant> settings, uint idx)
{
    m_transformList[idx]->setSettings(settings);
}

QMap<QString, QVariant> TransformManager::getSettings(uint idx)
{
    return m_transformList[idx]->getSettings();
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
        return;
    }
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader.instance();
        if(plugin){
            ITransform* algorithm = qobject_cast<ITransform*>(plugin);
            if(algorithm){
                m_transformList.push_back(new ITransformRequestDequeue(algorithm));
            } else {
                pluginLoader.unload();
            }

        }
    }
}
