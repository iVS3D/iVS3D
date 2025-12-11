#pragma once

#include <QObject>

#include "samplingwidget.h"
#include "datamanager.h"
#include "pluginmanager.h"

class PluginController : public QObject
{
    Q_OBJECT

public:
    explicit PluginController(DataManager* dataManager, SamplingWidget* samplingWidget);
    ~PluginController();

private slots:
    void slot_selectPlugin(QString name);
    void slot_enablePreview(bool enabled);
    void slot_startSelection();

private:
    DataManager* m_dataManager;
    SamplingWidget* m_samplingWidget;
    PluginHandle m_currentPlugin;
};