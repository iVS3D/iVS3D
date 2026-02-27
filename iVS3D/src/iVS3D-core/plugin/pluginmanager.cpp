#include "pluginmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <iostream>

PluginManager::~PluginManager()
{
    qDebug() << "Unloading plugins...";
    m_pluginThread.reset();
    for (auto& handle : m_plugins) {
        if (handle.loader) {
            handle.loader->unload();
            delete handle.loader;
        }
    }
}

PluginManager& PluginManager::instance() {
    static PluginManager INSTANCE;
    return INSTANCE;
}

QStringList PluginManager::getPluginNames() const {
    return m_plugins.keys();
}

bool PluginManager::hasPlugin(const QString& name) const { return m_plugins.contains(name); }

bool PluginManager::hasPreviewPlugin(const QString& name) const {
    auto it = m_plugins.find(name);
    return it != m_plugins.end() && it->hasPreview();
}

bool PluginManager::hasMaskPlugin(const QString& name) const {
    auto it = m_plugins.find(name);
    return it != m_plugins.end() && it->hasMask();
}

bool PluginManager::hasSelectionPlugin(const QString& name) const {
    auto it = m_plugins.find(name);
    return it != m_plugins.end() && it->hasSelection();
}

std::shared_ptr<QWidget> PluginManager::getSettingsWidget(const QString& pluginName) const {
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return nullptr;
    }
    return it->settingsWidget;
}

std::optional<Error> PluginManager::getSettingsWidgetError(
    const QString& pluginName) const {
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return std::nullopt;
    }
    return it->settingsWidgetError;
}

ApplySettingsResult PluginManager::applyPluginSettings(
    const QString& pluginName,
    const QMap<QString, QVariant>& settings) const {
    if (!m_pluginThread) {
        return tl::unexpected(Error(ErrorCode::RuntimeError, "PluginThread not available"));
    }
    return m_pluginThread->applyPluginSettings(pluginName, settings);
}

QMap<QString, QVariant> PluginManager::getPluginSettings(
    const QString& pluginName) const {
    if (!m_pluginThread) {
        return {};
    }
    return m_pluginThread->getPluginSettings(pluginName);
}

QString PluginManager::getPluginSettingsString(const QString& pluginName) const {
    if (!m_pluginThread) {
        return {};
    }
    return m_pluginThread->getPluginSettingsString(pluginName);
}

std::shared_ptr<PluginThread> PluginManager::getPluginThread() const {
    return m_pluginThread;
}

QVector<QPair<QString, Error>> PluginManager::loadSettingsWidgets() {
    QVector<QPair<QString, Error>> errors;

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        PluginHandle& handle = it.value();
        if (!handle.base) {
            continue;
        }

        // already loaded successfully
        if (handle.settingsWidget) {
            continue;
        }

        auto settingsWidgetResult = handle.base->getSettingsWidget();
        if (settingsWidgetResult) {
            auto settingsWidget = std::move(settingsWidgetResult.value());
            handle.settingsWidget =
                std::shared_ptr<QWidget>(std::move(settingsWidget));
            handle.settingsWidgetError = std::nullopt;
        } else {
            handle.settingsWidgetError = settingsWidgetResult.error();
            errors.append(QPair<QString, Error>(handle.name(), settingsWidgetResult.error()));
        }
    }

    return errors;
}

void PluginManager::enableCuda(bool useCuda) {
    if (m_pluginThread) {
        m_pluginThread->enableCuda(useCuda);
    }
}

PluginManager::PluginManager() {
    loadPlugins();
    m_pluginThread = std::make_shared<PluginThread>(m_plugins, this);
}

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
        handle.selection = qobject_cast<ISelection*>(handle.qobject);

        m_plugins.insert(handle.name(), handle);
        // print info like name and supported interfaces
        std::cout << "[INFO] Loaded plugin: " << handle.name().toStdString() << " | Supports Preview: " 
                  << (handle.hasPreview() ? "Yes" : "No") << " | Supports Mask: "
                  << (handle.hasMask() ? "Yes" : "No") << std::endl;

        
    }
}
