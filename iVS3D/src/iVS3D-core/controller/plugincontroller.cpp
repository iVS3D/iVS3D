#include "plugincontroller.h"

#include "applicationsettings.h"
PluginController::PluginController(DataManager* dataManager,
                                   SamplingWidget* samplingWidget,
                                   VideoPlayerController* vpc,
                                   std::shared_ptr<MaskStack> maskStack)
    : m_dataManager(dataManager),
      m_samplingWidget(samplingWidget),
      m_vpc(vpc),
      m_maskStack(maskStack) {
    connect(m_samplingWidget, &SamplingWidget::sig_selectedPluginChanged, this,
            &PluginController::slot_selectPlugin);
    connect(m_samplingWidget, &SamplingWidget::sig_enablePreviewChanged, this,
            &PluginController::slot_enablePreview);
    connect(m_samplingWidget, &SamplingWidget::sig_startSampling, this,
            &PluginController::slot_startSelection);
    connect(m_vpc, &VideoPlayerController::sig_disablePreview,
            [=]() { m_samplingWidget->setPreviewEnabled(false); });
    connect(m_samplingWidget, &SamplingWidget::sig_addMask, this,
            &PluginController::slot_addMask);

    m_currentPlugin = PluginHandle{nullptr, nullptr, nullptr};

    auto plugin_names = PluginManager::instance().getPlugins();
    QStringList name_list;
    for (const auto& plugin : plugin_names) {
        name_list.append(plugin.name());
    }
    m_samplingWidget->setPluginList(name_list);
    auto name = ApplicationSettings::instance().getDefaultPluginName();
    printf("[PluginController] Loading default plugin name: %s\n",
           name.toStdString().c_str());
    if (!name.isEmpty() && name_list.contains(name)) {
        m_samplingWidget->setSelectedPlugin(name);
    } else {
        name = name_list.isEmpty() ? "" : name_list.first();
        m_samplingWidget->setSelectedPlugin(name);
    }

    if (!name_list.isEmpty()) {
        m_samplingWidget->setEnabled(true);
        slot_selectPlugin(name);
    }
}

PluginController::~PluginController() {
    if (m_currentPlugin.base) m_currentPlugin.base->deactivate();

    m_samplingWidget->setEnabled(false);
    printf("[PluginController] Storing default plugin name: %s\n",
           m_currentPlugin.name().toStdString().c_str());
    ApplicationSettings::instance().setDefaultPluginName(
        m_currentPlugin.name());
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

void PluginController::slot_startSelection() {
    printf("[PluginController] Starting selection with following plugin: %s\n",
           m_currentPlugin.name().toStdString().c_str());
}

void PluginController::slot_addMask() {
    printf("[PluginController] Adding mask with following plugin: %s\n",
           m_currentPlugin.name().toStdString().c_str());

    assert(m_maskStack != nullptr);
    MaskRecord record;
    record.pluginName = m_currentPlugin.name();
    record.pluginSettings =
        m_currentPlugin.base->getSettings();  // get current plugin settings
    record.workingResolution = m_dataManager->getModelInputPictures()
                     ->getReaderParams()
                     ->getWorkingResolution();
    record.roi =
        m_dataManager->getModelInputPictures()->getReaderParams()->getUseRoi()
            ? m_dataManager->getModelInputPictures()->getReaderParams()->getRoi()
            : ROI();  // get current ROI or empty ROI if not used
    m_maskStack->addRecord(record);
}

void PluginController::slot_selectPlugin(QString name) {
    auto plugin_handle = PluginManager::instance().getPluginByName(name);
    if (!plugin_handle) return;

    if (m_currentPlugin.base) m_currentPlugin.base->deactivate();

    // hide all plugin action buttons initially
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::ALL_ACTIONS, false);
    m_samplingWidget->setPreviewEnabled(false);

    m_currentPlugin = *plugin_handle;
    auto result = m_currentPlugin.base->getSettingsWidget(m_samplingWidget);
    if (!result) {
        std::shared_ptr<QWidget> errorWidget = std::make_shared<QWidget>();
        errorWidget->setLayout(new QVBoxLayout());
        QLabel* errorLabel =
            new QLabel(tr("The plugin encountered an error:\n%1")
                           .arg(result.error().message));
        errorWidget->layout()->addWidget(errorLabel);
        m_samplingWidget->showPluginSettings(errorWidget);
        return;
    }
    m_samplingWidget->showPluginSettings(result.value());

    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::PREVIEW_TOGGLE,
        m_currentPlugin.hasPreview());
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::ADD_MASK, m_currentPlugin.hasMask());
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::START_SELECTION,
        m_currentPlugin.hasSelection());

    m_currentPlugin.base->activate();
}