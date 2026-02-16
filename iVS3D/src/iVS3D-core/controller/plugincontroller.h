#pragma once

#include <QObject>

#include "samplingwidget.h"
#include "DataManager.h"
#include "pluginmanager.h"
#include "videoplayercontroller.h"
#include "maskstack.h"
#include "pluginthread.h"
#include "stackcontroller.h"

class PluginController : public QObject
{
    Q_OBJECT

public:
    explicit PluginController(DataManager* dataManager, SamplingWidget* samplingWidget, VideoPlayerController* vpc, StackController* stackController, std::shared_ptr<PluginThread> pluginThread, std::shared_ptr<MaskStack> maskStack = nullptr);
    ~PluginController();

public slots:
    void slot_selectPlugin(QString name);
    void slot_pausePreview();
    void slot_resumePreview();
    void slot_disableSampling();
    void slot_enableSampling();

private slots:
    void slot_enablePreview(bool enabled);
    void slot_startSelection();
    void slot_addMask();
    void slot_previewStateChanged(const PreviewState& state);

private:
    DataManager* m_dataManager;
    SamplingWidget* m_samplingWidget;
    VideoPlayerController* m_vpc;
    PluginHandle m_currentPlugin;
    StackController* m_stack;
    std::shared_ptr<MaskStack> m_maskStack;
    std::shared_ptr<PluginThread> m_pluginThread;
    volatile bool m_selectionCancelFlag;
    bool m_previewWasEnabled = false;
    bool m_samplingWasEnabled = false;
};