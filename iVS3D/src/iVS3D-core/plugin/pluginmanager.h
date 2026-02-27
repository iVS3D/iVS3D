#pragma once

#include <QPluginLoader>
#include <QHash>
#include <QString>
#include <QVector>
#include <QObject>

#include <optional>
#include <memory>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"
#include "iselection.h"
#include "pluginhandle.h"
#include "pluginthread.h"

class PluginManager : public QObject {
    Q_OBJECT

public:
    ~PluginManager();
    static PluginManager& instance();

    QStringList getPluginNames() const;
    bool hasPlugin(const QString& name) const;
    bool hasPreviewPlugin(const QString& name) const;
    bool hasMaskPlugin(const QString& name) const;
    bool hasSelectionPlugin(const QString& name) const;

    std::shared_ptr<QWidget> getSettingsWidget(const QString& pluginName) const;
    std::optional<Error> getSettingsWidgetError(const QString& pluginName) const;

    ApplySettingsResult applyPluginSettings(
        const QString& pluginName,
        const QMap<QString, QVariant>& settings) const;
    QMap<QString, QVariant> getPluginSettings(const QString& pluginName) const;
    QString getPluginSettingsString(const QString& pluginName) const;

    std::shared_ptr<PluginThread> getPluginThread() const;

    /**
     * @brief loadSettingsWidgets asks each plugin to create its settings widget once.
     *
     * This should only be called in GUI mode. In headless mode, do not call it.
     *
     * @return A list of (pluginName, error) pairs for plugins that failed to
     * create their settings widget.
     */
    QVector<QPair<QString, Error>> loadSettingsWidgets();

    void enableCuda(bool useCuda);

private:
    PluginManager();
    void loadPlugins();

    QHash<QString, PluginHandle> m_plugins;
    std::shared_ptr<PluginThread> m_pluginThread;
};
