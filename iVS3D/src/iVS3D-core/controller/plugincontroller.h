#pragma once

#include <QObject>

#include "samplingwidget.h"
#include "DataManager.h"
#include "pluginmanager.h"
#include "videoplayercontroller.h"
#include "maskstack.h"

class PluginController : public QObject
{
    Q_OBJECT

public:
    explicit PluginController(DataManager* dataManager, SamplingWidget* samplingWidget, VideoPlayerController* vpc, std::shared_ptr<MaskStack> maskStack = nullptr);
    ~PluginController();

public slots:
    void slot_selectPlugin(QString name);

private slots:
    void slot_enablePreview(bool enabled);
    void slot_startSelection();
    void slot_addMask();

private:
    DataManager* m_dataManager;
    SamplingWidget* m_samplingWidget;
    VideoPlayerController* m_vpc;
    PluginHandle m_currentPlugin;
    std::shared_ptr<MaskStack> m_maskStack;
};