#include "plugincontroller.h"

PluginController::PluginController(DataManager* dataManager,
                                   SamplingWidget* samplingWidget, VideoPlayerController* vpc)
    : m_dataManager(dataManager), m_samplingWidget(samplingWidget), m_vpc(vpc) {
    connect(m_samplingWidget, &SamplingWidget::sig_selectedPluginChanged, this,
            &PluginController::slot_selectPlugin);
    connect(m_samplingWidget, &SamplingWidget::sig_enablePreviewChanged, this,
            &PluginController::slot_enablePreview);
    connect(m_samplingWidget, &SamplingWidget::sig_startSampling, this,
            &PluginController::slot_startSelection);
    

    m_currentPlugin = PluginHandle{nullptr, nullptr, nullptr};

    auto plugin_names = PluginManager::instance().getPlugins();
    QStringList name_list;
    for (const auto& plugin : plugin_names) {
        name_list.append(plugin.name());
    }
    m_samplingWidget->setPluginList(name_list);
    m_samplingWidget->setSelectedPlugin(name_list.isEmpty() ? "" : name_list.first());

    if (!name_list.isEmpty()) {
        m_samplingWidget->setEnabled(true);
        slot_selectPlugin(name_list.first());
    }

}

PluginController::~PluginController() {
    if (m_currentPlugin.base)
        m_currentPlugin.base->deactivate();

    m_samplingWidget->setEnabled(false);
}

void PluginController::slot_enablePreview(bool enabled) {
    if (enabled && m_currentPlugin.hasPreview()) {
        // when enabled the video player controller gets the preview plugin
        // and needs to update the preview on requests
        m_vpc->setPreviewPlugin(m_currentPlugin.preview);
        connect(m_currentPlugin.base, &IBase::updatePreview, m_vpc,
                &VideoPlayerController::slot_refreshPreview);
    } else {
        // when disabled the video player controller removes the preview plugin
        // and disconnects the updatePreview signal
        m_vpc->setPreviewPlugin(nullptr);
        disconnect(m_currentPlugin.base, &IBase::updatePreview, m_vpc,
                   &VideoPlayerController::slot_refreshPreview);
    }
    m_vpc->resetLayout();
    m_vpc->slot_redraw();
}

void PluginController::slot_startSelection() {}

void PluginController::slot_selectPlugin(QString name) {
    auto plugin_handle = PluginManager::instance().getPluginByName(name);
    if (!plugin_handle) return;

    if (m_currentPlugin.base)
        m_currentPlugin.base->deactivate();

    m_samplingWidget->disablePreview();

    m_currentPlugin = *plugin_handle;
    m_samplingWidget->showAlgorithmSettings(
        m_currentPlugin.base->getSettingsWidget(m_samplingWidget));

    m_samplingWidget->setPreviewVisible(m_currentPlugin.hasPreview());
    m_samplingWidget->setSelectionVisible(m_currentPlugin.hasSelection());

    m_currentPlugin.base->activate();
}