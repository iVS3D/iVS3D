#include "plugincontroller.h"

PluginController::PluginController(DataManager* dataManager,
                                   SamplingWidget* samplingWidget)
    : m_dataManager(dataManager), m_samplingWidget(samplingWidget) {
    connect(m_samplingWidget, &SamplingWidget::sig_selectedPluginChanged, this,
            &PluginController::slot_selectPlugin);
    
    m_currentPlugin = PluginHandle{nullptr, nullptr, nullptr};

    auto plugin_names = PluginManager::instance().getPlugins();
    QStringList name_list;
    for (const auto& plugin : plugin_names) {
        name_list.append(plugin.name());
    }
    m_samplingWidget->setPluginList(name_list);
    m_samplingWidget->setSelectedPlugin(name_list.isEmpty() ? "" : name_list.first());
}

PluginController::~PluginController() {}

void PluginController::slot_enablePreview(bool enabled) {}

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