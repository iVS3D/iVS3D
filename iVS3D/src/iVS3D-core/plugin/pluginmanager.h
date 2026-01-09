#pragma once

#include <QPluginLoader>
#include <QHash>
#include <QString>
#include <QVector>

#include <optional>

#include "ibase.h"
#include "imask.h"
#include "ipreview.h"

struct PluginHandle {
    QPluginLoader* loader;
    IBase* base;
    QObject* qobject;
    IPreview* preview = nullptr;
    IMask* mask = nullptr;

    QString name() const { return base ? base->getName() : QString(); }

    bool hasPreview() const { return preview != nullptr; }

    bool hasMask() const { return mask != nullptr; }

    bool hasSelection() const { return false; }
};

class PluginManager : public QObject {
    Q_OBJECT

public:
    static PluginManager& instance();

    QVector<PluginHandle> getPlugins() const;
    QStringList getPluginNames() const;
    QVector<PluginHandle> getMaskPlugins() const;
    std::optional<PluginHandle> getPluginByName(const QString& name) const;

    void enableCuda(bool useCuda);

private:
    PluginManager();
    void loadPlugins();

    QHash<QString, PluginHandle> m_plugins;
};
