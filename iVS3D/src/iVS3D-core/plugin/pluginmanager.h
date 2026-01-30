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
    static PluginManager& instance();

    QVector<PluginHandle> getPlugins() const;
    QStringList getPluginNames() const;
    QVector<PluginHandle> getMaskPlugins() const;
    QVector<PluginHandle> getSelectionPlugins() const;
    std::optional<PluginHandle> getPluginByName(const QString& name) const;

    void enableCuda(bool useCuda);

private:
    PluginManager();
    void loadPlugins();

    QHash<QString, PluginHandle> m_plugins;
};
