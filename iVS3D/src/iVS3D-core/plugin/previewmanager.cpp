#include "previewmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <iostream>

PreviewManager& PreviewManager::instance() {
    static PreviewManager INSTANCE;
    return INSTANCE;
}

PreviewManager::PreviewManager() {
    loadPlugins();
}

void PreviewManager::loadPlugins() {
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
    std::cout << "[INFO] Loading preview plugins from: " << pluginsDir.absolutePath().toStdString() << std::endl;
    const auto entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader *pluginLoader = new QPluginLoader(pluginsDir.absoluteFilePath(fileName));
        auto *plugin = pluginLoader->instance();
        if(plugin){
            IPreview* preview = qobject_cast<IPreview*>(plugin);
            IBase* base = qobject_cast<IBase*>(plugin);
            if(preview && base){
                m_previewList.push_back({ preview, base, pluginLoader });
                std::cout << "[INFO] Loaded preview plugin: " << base->getName().toStdString() << std::endl;
            } else {
                pluginLoader->unload();
                delete pluginLoader;
            }
        } else {
            delete pluginLoader;
        }
    }

    for (auto& p : m_previewList){
        QObject::connect(p.base, &IBase::updatePreview, this, &PreviewManager::updatePreviewRequest);
    }

    for (const auto& p : m_previewList) {
        auto v = p.preview->generatePreview({10, cv::Mat()}); // Test call with dummy data
        std::cout << "[INFO] Generated preview with " << v.views.size() << " views." << std::endl;
    }
}

void PreviewManager::updatePreviewRequest() {
    std::cout << "[INFO] Received preview update request." << std::endl;
}