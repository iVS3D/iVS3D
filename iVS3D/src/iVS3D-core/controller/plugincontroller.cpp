#include "plugincontroller.h"

#include "applicationsettings.h"
#include "logmanager.h"
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
    connect(m_pluginThread.get(), &PluginThread::activePluginUpdatePreview, m_vpc,
            &VideoPlayerController::slot_refreshPreview, Qt::QueuedConnection);

    auto reader = m_dataManager->getModelInputPictures()->getReader();
    m_pluginThread->onInputLoaded(reader);
    auto *md = m_dataManager->getModelInputPictures()->getReader()->getMetaData();
    if (md) {
        InputMetaData inputMetaData{ md };
        m_pluginThread->onMetaDataLoaded(inputMetaData);
    }

    m_currentPluginName.clear();

    QStringList name_list = PluginManager::instance().getPluginNames();
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
    m_pluginThread->clearActivePlugin();

    m_samplingWidget->setEnabled(false);
    printf("[PluginController] Storing default plugin name: %s\n",
           m_currentPluginName.toStdString().c_str());
    ApplicationSettings::instance().setDefaultPluginName(
        m_currentPluginName);
}

void PluginController::slot_enablePreview(bool enabled) {
    if (enabled && m_currentHasPreview) {
        // when enabled the video player controller gets the preview plugin
        // and needs to update the preview on requests
        m_vpc->setPreviewPlugin(m_currentPluginName);
    } else {
        // when disabled the video player controller removes the preview plugin
        m_vpc->clearPreviewPlugin();
    }
    m_vpc->resetLayout();
    m_vpc->slot_redraw();
}

void PluginController::slot_startSelection() {
    printf("[PluginController] Starting selection with following plugin: %s\n",
           m_currentPluginName.toStdString().c_str());

    if (!m_currentHasSelection) return;
    
    m_vpc->slot_stopPlay(); // stop playback if running

    // Prepare selection data
    SelectionData data;
    data.selectedIndices = m_dataManager->getModelInputPictures()->getAllKeyframes(true);
    data.roi = m_dataManager->getModelInputPictures()->getReaderParams()->getRoi();
    data.workingResolution = m_dataManager->getModelInputPictures()
                                 ->getReaderParams()
                                 ->getWorkingResolution();
    data.reader = m_dataManager->getModelInputPictures()->getReader();
    auto selectionLog =
        LogManager::instance().createLogFile(m_currentPluginName, true);
    if (selectionLog) {
        selectionLog->setSettings(
            PluginManager::instance().getPluginSettings(m_currentPluginName));
        selectionLog->setInputInfo(data.selectedIndices);
        data.logFile = selectionLog;
    }
                                    
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
        m_pluginThread->cancelSelection();
    });
    
    // Connect progress updates (works across thread boundaries in Qt)
    connect(m_pluginThread.get(), &PluginThread::activePluginUpdateProgress, progressBar,
            [progressBar](int progress, QString message) {
                progressBar->setValue(progress);
                if (!message.isEmpty()) {
                    progressBar->setFormat(message + " (%p%)");
                }
            }, Qt::QueuedConnection);

    progressDialog->show();

    auto selectionFinishedConnection =
        std::make_shared<QMetaObject::Connection>();
    *selectionFinishedConnection = connect(
        m_pluginThread.get(), &PluginThread::selectionFinished, this,
        [this, progressDialog, selectionLog,
         selectionFinishedConnection](const SelectionResultData& result) {
        if (result.pluginName != m_currentPluginName) {
            return;
        }

        disconnect(*selectionFinishedConnection);

        progressDialog->close();
        progressDialog->deleteLater();

        if (m_selectionCancelFlag) {
            // Selection was cancelled
            QMessageBox::information(
                m_samplingWidget, tr("Selection Cancelled"),
                tr("The selection process was cancelled by the user."));
            return;
        }

        if (!result.success) {
            QMessageBox::critical(
                m_samplingWidget, tr("Selection Error"),
                tr("An error occurred during selection:\n%1")
                    .arg(result.errorMessage));
        } else {
            auto selectedIndices = result.selectedIndices;
            if (selectionLog) {
                selectionLog->setResultsInfo(selectedIndices);
            }
            m_dataManager->getModelInputPictures()->updateMIP(selectedIndices);
            const auto settings =
                PluginManager::instance().getPluginSettings(m_currentPluginName);
            const auto settingsString =
                PluginManager::instance().getPluginSettingsString(
                    m_currentPluginName);
            m_stack->addToStack(m_currentPluginName, settings, settingsString);
            m_dataManager->getHistory()->slot_save();
        }
    });

    m_pluginThread->requestSelection(m_currentPluginName, data);
}

void PluginController::slot_addMask() {
    printf("[PluginController] Adding mask with following plugin: %s\n",
           m_currentPluginName.toStdString().c_str());

    assert(m_maskStack != nullptr);
    MaskRecord record;
    record.pluginName = m_currentPluginName;
    record.pluginSettings =
        PluginManager::instance().getPluginSettings(m_currentPluginName);
    record.pluginSettingsString =
        PluginManager::instance().getPluginSettingsString(m_currentPluginName);
    record.workingResolution = m_dataManager->getModelInputPictures()
                     ->getReaderParams()
                     ->getWorkingResolution();
    record.roi =
        m_dataManager->getModelInputPictures()->getReaderParams()->getUseRoi()
            ? m_dataManager->getModelInputPictures()->getReaderParams()->getRoi()
            : ROI();  // get current ROI or empty ROI if not used
    m_maskStack->addRecord(record);
}

void PluginController::slot_pausePreview() {
    if (m_samplingWidget) {
        m_previewWasEnabled = m_samplingWidget->isPreviewEnabled();
        m_samplingWidget->setPreviewEnabled(false);
    }
}

void PluginController::slot_resumePreview() {
    if (m_samplingWidget && m_previewWasEnabled) {
        m_samplingWidget->setPreviewEnabled(true);
    }
}

void PluginController::slot_disableSampling() {
    if (m_samplingWidget) {
        m_samplingWasEnabled = m_samplingWidget->isEnabled();
        m_samplingWidget->setEnabled(false);
    }
}

void PluginController::slot_enableSampling() {
    if (m_samplingWidget && m_samplingWasEnabled) {
        m_samplingWidget->setEnabled(true);
    }
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
    if (!PluginManager::instance().hasPlugin(name)) return;

    const bool previewWasEnabled = m_samplingWidget->isPreviewEnabled();

    // Tear down previous preview wiring before switching plugin.
    if (previewWasEnabled) {
        m_vpc->clearPreviewPlugin();
    }

    if (!m_pluginThread->setActivePlugin(name)) {
        return;
    }

    // hide all plugin action buttons initially
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::ALL_ACTIONS, false);

    m_currentPluginName = name;
    m_currentHasPreview = PluginManager::instance().hasPreviewPlugin(name);
    m_currentHasMask = PluginManager::instance().hasMaskPlugin(name);
    m_currentHasSelection = PluginManager::instance().hasSelectionPlugin(name);

    auto settingsWidget = PluginManager::instance().getSettingsWidget(name);
    if (!settingsWidget) {
        QWidget* errorWidget = new QWidget(m_samplingWidget);
        errorWidget->setLayout(new QVBoxLayout());
        auto settingsWidgetError =
            PluginManager::instance().getSettingsWidgetError(name);
        const QString message =
            settingsWidgetError
                ? settingsWidgetError->message
                : tr("No settings widget available for this plugin.");
        QLabel* errorLabel =
            new QLabel(tr("The plugin encountered an error:\n%1").arg(message));
        errorWidget->layout()->addWidget(errorLabel);
        m_samplingWidget->showPluginSettings(errorWidget);
        return;
    }
    m_samplingWidget->showPluginSettings(settingsWidget.get());

    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::PREVIEW_TOGGLE,
        m_currentHasPreview);
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::ADD_MASK, m_currentHasMask);
    m_samplingWidget->setPluginActionVisible(
        SamplingWidget::PluginActions::START_SELECTION,
        m_currentHasSelection);

    const bool keepPreviewEnabled = previewWasEnabled && m_currentHasPreview;
    m_samplingWidget->setPreviewEnabled(keepPreviewEnabled);
    // If preview stayed enabled across switch, checkbox state didn't change and
    // therefore no signal is emitted -> wire preview explicitly.
    if (keepPreviewEnabled) {
        slot_enablePreview(true);
    } else {
        slot_enablePreview(false);
    }
}