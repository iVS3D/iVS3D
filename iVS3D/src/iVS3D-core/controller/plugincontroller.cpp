#include "plugincontroller.h"
#include <QtConcurrent>

#include "applicationsettings.h"
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>

PluginController::PluginController(DataManager* dataManager,
                                   SamplingWidget* samplingWidget,
                                   VideoPlayerController* vpc,
                                   StackController* stackController,
                                   std::shared_ptr<PluginThread> pluginThread,
                                   std::shared_ptr<MaskStack> maskStack)
    : m_dataManager(dataManager),
      m_samplingWidget(samplingWidget),
      m_vpc(vpc),
      m_stack(stackController),
      m_pluginThread(pluginThread),
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
    connect(m_pluginThread.get(), &PluginThread::previewStateChanged, this,
            &PluginController::slot_previewStateChanged);

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
        m_vpc->setPreviewPlugin(m_currentPlugin);
        connect(m_currentPlugin.base, &IBase::updatePreview, m_vpc,
                &VideoPlayerController::slot_refreshPreview);
    } else {
        // when disabled the video player controller removes the preview plugin
        // and disconnects the updatePreview signal
        m_vpc->clearPreviewPlugin();
        disconnect(m_currentPlugin.base, &IBase::updatePreview, m_vpc,
                   &VideoPlayerController::slot_refreshPreview);
    }
    m_vpc->resetLayout();
    m_vpc->slot_redraw();
}

void PluginController::slot_startSelection() {
    printf("[PluginController] Starting selection with following plugin: %s\n",
           m_currentPlugin.name().toStdString().c_str());

    if (!m_currentPlugin.hasSelection()) return;
    
    m_samplingWidget->setPreviewEnabled(false);
    m_vpc->slot_stopPlay(); // stop playback if running

    // Prepare selection data
    SelectionData data;
    data.selectedIndices = m_dataManager->getModelInputPictures()->getAllKeyframes(true);
    data.roi = m_dataManager->getModelInputPictures()->getReaderParams()->getRoi();
    data.workingResolution = m_dataManager->getModelInputPictures()
                                 ->getReaderParams()
                                 ->getWorkingResolution();
    data.reader = m_dataManager->getModelInputPictures()->getReader();
                                    
    // Reset cancel flag
    m_selectionCancelFlag = false;
    
    // Create progress dialog
    QDialog* progressDialog = new QDialog(m_samplingWidget);
    progressDialog->setWindowTitle(tr("Selection in progress"));
    progressDialog->setModal(true);
    QVBoxLayout* layout = new QVBoxLayout(progressDialog);
    QLabel* label = new QLabel(tr("Please wait while the selection is being processed..."));
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    layout->addWidget(label);
    layout->addWidget(progressBar);
    
    // Add cancel button
    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    layout->addWidget(progressBar);
    layout->addWidget(cancelButton);
    progressDialog->setLayout(layout);
    
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        m_selectionCancelFlag = true;
    });
    
    // Connect progress updates (works across thread boundaries in Qt)
    connect(m_currentPlugin.base, &IBase::updateProgress, progressBar,
            [progressBar](int progress, QString message) {
                progressBar->setValue(progress);
                if (!message.isEmpty()) {
                    progressBar->setFormat(message + " (%p%)");
                }
            }, Qt::QueuedConnection);

    progressDialog->show();
    
    // Run the expensive operation in a worker thread
    auto selectionFunction = [this, data]() {
        return m_currentPlugin.selection->selectImages(data, m_selectionCancelFlag);
    };
    
    QFuture<decltype(m_currentPlugin.selection->selectImages(data, m_selectionCancelFlag))> future = 
        QtConcurrent::run(QThreadPool::globalInstance(), selectionFunction);
    
    // Monitor completion with QFutureWatcher
    auto* watcher = new QFutureWatcher<decltype(m_currentPlugin.selection->selectImages(data, m_selectionCancelFlag))>(this);
    
    connect(watcher, &QFutureWatcherBase::finished, this, [this, progressDialog, watcher]() {
        progressDialog->close();
        progressDialog->deleteLater();
        
        auto result = watcher->result();
        if (m_selectionCancelFlag) {
            // Selection was cancelled
            QMessageBox::information(
                m_samplingWidget, tr("Selection Cancelled"),
                tr("The selection process was cancelled by the user."));
            watcher->deleteLater();
            return;
        }
        
        if (!result) {
            QMessageBox::critical(
                m_samplingWidget, tr("Selection Error"),
                tr("An error occurred during selection:\n%1")
                    .arg(result.error().message));
        } else {
            auto selectedIndices = result.value();
            m_dataManager->getModelInputPictures()->updateMIP(selectedIndices);
            m_stack->addToStack(m_currentPlugin);
            m_dataManager->getHistory()->slot_save();
        }
        
        watcher->deleteLater();
    });
    
    watcher->setFuture(future);
}

void PluginController::slot_addMask() {
    printf("[PluginController] Adding mask with following plugin: %s\n",
           m_currentPlugin.name().toStdString().c_str());

    assert(m_maskStack != nullptr);
    MaskRecord record;
    record.pluginName = m_currentPlugin.name();
    record.pluginSettings =
        m_currentPlugin.base->getSettings();  // get current plugin settings
    record.pluginSettingsString = m_currentPlugin.base->getSettingsString();
    record.workingResolution = m_dataManager->getModelInputPictures()
                     ->getReaderParams()
                     ->getWorkingResolution();
    record.roi =
        m_dataManager->getModelInputPictures()->getReaderParams()->getUseRoi()
            ? m_dataManager->getModelInputPictures()->getReaderParams()->getRoi()
            : ROI();  // get current ROI or empty ROI if not used
    m_maskStack->addRecord(record);
}

void PluginController::slot_previewStateChanged(const PreviewState& state) {
    switch(state) {
        case PreviewState::Idle:
            m_samplingWidget->setPreviewState(SamplingWidget::PreviewState::Idle);
            break;
        case PreviewState::Processing:
            m_samplingWidget->setPreviewState(SamplingWidget::PreviewState::Processing);
            break;
        default:
            break;
    }
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