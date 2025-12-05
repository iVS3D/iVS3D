#pragma once

#include <QObject>

#include "samplingwidget.h"
#include "DataManager.h"
#include "pluginmanager.h"
#include "videoplayercontroller.h"

class PluginController : public QObject
{
    Q_OBJECT

public:
    explicit PluginController(DataManager* dataManager, SamplingWidget* samplingWidget, VideoPlayerController* vpc);
    ~PluginController();

private slots:
    void slot_selectPlugin(QString name);
    void slot_enablePreview(bool enabled);
    void slot_startSelection();

private:
    DataManager* m_dataManager;
    SamplingWidget* m_samplingWidget;
    VideoPlayerController* m_vpc;
    PluginHandle m_currentPlugin;
};