#include "pluginmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>

PluginManager& PluginManager::instance() {
    static PluginManager INSTANCE;
    return INSTANCE;
}

QVector<PluginHandle> PluginManager::getPlugins() const {
    return m_plugins.values().toVector();
}

QStringList PluginManager::getPluginNames() const {
    return m_plugins.keys();
}

QVector<PluginHandle> PluginManager::getMaskPlugins() const {
    QVector<PluginHandle> maskPlugins;
    for (const PluginHandle& handle : m_plugins.values()) {
        if (handle.hasMask()) {
            maskPlugins.append(handle);
        }
    }
    return maskPlugins;
}

std::optional<PluginHandle> PluginManager::getPluginByName(const QString& name) const {
    if (!m_plugins.contains(name)) {
        return std::nullopt;
    }
    return m_plugins.value(name);
}

void PluginManager::enableCuda(bool useCuda) {
    for (auto& handle : m_plugins) {
        IBase* base = handle.base;
        if (base) {
            base->onCudaChanged(useCuda);
        }
    }
}

PluginManager::PluginManager() { loadPlugins(); }

void PluginManager::loadPlugins() {
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
        std::cerr << "\n[ERROR] No plugins directory found at: " << pluginsDir.absolutePath().toStdString() << std::endl;
        return;
    }
    std::cout << "\n[INFO] Loading plugins from: " << pluginsDir.absolutePath().toStdString() << std::endl;
    const auto entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        PluginHandle handle;
        handle.loader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        handle.qobject = handle.loader->instance();

        if (!handle.qobject) {
            delete handle.loader;
            continue;
        }

        handle.base = qobject_cast<IBase*>(handle.qobject);
        if (!handle.base) {
            handle.loader->unload();
            delete handle.loader;
            continue;
        }

        handle.preview = qobject_cast<IPreview*>(handle.qobject);
        handle.mask = qobject_cast<IMask*>(handle.qobject);
        m_plugins.insert(handle.name(), handle);
        // print info like name and supported interfaces
        std::cout << "[INFO] Loaded plugin: " << handle.name().toStdString() << " | Supports Preview: " 
                  << (handle.hasPreview() ? "Yes" : "No") << " | Supports Mask: "
                  << (handle.hasMask() ? "Yes" : "No") << std::endl;

        
    }
}
