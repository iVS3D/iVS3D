#pragma once

#include <QPluginLoader>
#include <QHash>
#include <QString>
#include <QVector>

#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"
#include "iselection.h"
#include "pluginhandle.h"

class PluginManager : public QObject {
    Q_OBJECT

public:
    ~PluginManager();
    static PluginManager& instance();

    QVector<PluginHandle> getPlugins() const;
    QStringList getPluginNames() const;
    QVector<PluginHandle> getMaskPlugins() const;
    QVector<PluginHandle> getSelectionPlugins() const;
    std::optional<PluginHandle> getPluginByName(const QString& name) const;

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
};
