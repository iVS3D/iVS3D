#include "algorithmcontroller.h"


AlgorithmController::AlgorithmController(DataManager* dataManager, SamplingWidget *samplingWidget)
{
    m_dataManager = dataManager;
    m_pluginIdx = samplingWidget->getSelectedAlgorithm();
    m_pluginType = (samplingWidget->getSelctedType() ? PluginType::Algorithm : PluginType::Transform);
    //Save samplingSettings and connect
    m_samplingWidget = samplingWidget;
    m_samplingWidget->setEnabled(true);
    connect(m_samplingWidget, &SamplingWidget::sig_selectedAlgorithmChanged, this, &AlgorithmController::slot_selectAlgorithm);
    connect(m_samplingWidget, &SamplingWidget::sig_selectedTransformChanged, this, &AlgorithmController::slot_selectTransform);
    connect(m_samplingWidget, &SamplingWidget::sig_startSampling, this, &AlgorithmController::slot_startAlgorithm);
    connect(m_samplingWidget, &SamplingWidget::sig_startGenerateSettings, this, &AlgorithmController::slot_startGenerateSettings);
    connect(m_samplingWidget, &SamplingWidget::sig_enablePreviewChanged, this, &AlgorithmController::slot_previewStateChanged);
    //Create new AlgorithmExecutor
    m_algExec = new AlgorithmExecutor(m_dataManager->getModelInputPictures());
    connect(m_algExec, &AlgorithmExecutor::sig_pluginFinished, this, &AlgorithmController::slot_algorithmFinished);
    connect(m_algExec, &AlgorithmExecutor::sig_algorithmAborted, this, &AlgorithmController::slot_algorithmAborted);

    m_algorithmProgressDialog = nullptr;

    m_future = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);
    m_fQueue = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);

    connect(&AlgorithmManager::instance(), &AlgorithmManager::sig_updateKeyframe, this, &AlgorithmController::slot_updateKeyframes);
    connect(&AlgorithmManager::instance(), &AlgorithmManager::sig_updateBuffer, this, &AlgorithmController::slot_updateBuffer);
}

AlgorithmController::~AlgorithmController()
{
    m_samplingWidget->setEnabled(false);
    disconnect(&AlgorithmManager::instance(), &AlgorithmManager::sig_updateKeyframe, this, &AlgorithmController::slot_updateKeyframes);
    disconnect(&AlgorithmManager::instance(), &AlgorithmManager::sig_updateBuffer, this, &AlgorithmController::slot_updateBuffer);
}

void AlgorithmController::slot_selectAlgorithm(int idx)
{

    m_pluginIdx = idx;
    m_pluginType = PluginType::Algorithm;

    // show the settings widget of the new algo
    m_samplingWidget->disablePreview();
    m_samplingWidget->showAlgorithmSettings(AlgorithmManager::instance().getSettingsWidget(m_samplingWidget, idx));
    // clear queue for transformations
    m_fQueue = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE,NO_TRANSFORM);

    emit sig_hasStatusMessage(tr("Selected algorithm: ") + AlgorithmManager::instance().getPluginNameToIndex(idx));
    TransformManager::instance().selectTransform(UINT_MAX);
}

void AlgorithmController::slot_selectTransform(int idx)
{

    m_pluginIdx = idx;
    m_pluginType = PluginType::Transform;

    m_samplingWidget->showAlgorithmSettings(TransformManager::instance().getSettingsWidget(m_samplingWidget, idx));

    emit sig_hasStatusMessage(tr("Selected transformation: ") + TransformManager::instance().getTransformList()[idx]);
    TransformManager::instance().selectTransform(idx);
}

void AlgorithmController::slot_startAlgorithm()
{
    emit sig_stopPlay();

    if(m_pluginType != PluginType::Algorithm){
        return;
    }

    // clean up leaftovers from last algorithm
    if(m_algorithmProgressDialog){
        disconnect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, m_algExec, &AlgorithmExecutor::slot_abort);
        disconnect(m_algExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);
        delete m_algorithmProgressDialog;
    }

    // setup progress dialog ...
    m_algorithmProgressDialog = new ProgressDialog(m_samplingWidget,true);
    m_algorithmProgressDialog->setSizeGripEnabled(false);

    // ... and connect dialog to executor
    connect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, m_algExec, &AlgorithmExecutor::slot_abort);
    connect(m_algExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);

    m_processRunning = true;
    m_progressDialogActivated = false;

    // start algorithm and display progress dialog (delayed)
    m_timer = QElapsedTimer();
    m_timer.start();
    m_algExec->startSampling(m_pluginIdx);
    QTimer::singleShot(PROGRESS_DIALOG_DELAY_MS, this, &AlgorithmController::slot_showProgessDialog); // wait for 100ms before displaying the dialog
}

void AlgorithmController::slot_showProgessDialog()
{
    // if the process is still running then display the dialog
    if (m_processRunning && !m_progressDialogActivated){
        m_progressDialogActivated = true;
        m_algorithmProgressDialog->show();
    }
}

void AlgorithmController::slot_startGenerateSettings()
{
    emit sig_stopPlay();
    // upadate mip (boundaries)
    //emit sig_updateBoundaries();

    if(m_pluginType != PluginType::Algorithm){
        return;
    }

    // clean up leaftovers from last algorithm
    if(m_algorithmProgressDialog){
        disconnect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, m_algExec, &AlgorithmExecutor::slot_abort);
        disconnect(m_algExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);
        delete m_algorithmProgressDialog;
    }

    // setup progress dialog ...
    m_algorithmProgressDialog = new ProgressDialog(m_samplingWidget,true);
    m_algorithmProgressDialog->setSizeGripEnabled(false);

    // ... and connect dialog to executor
    connect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, m_algExec, &AlgorithmExecutor::slot_abort);
    connect(m_algExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);

    m_processRunning = true;
    m_progressDialogActivated = false;

    // display progress dialog and start algorithm
    m_timer = QElapsedTimer();
    m_timer.start();
    m_algExec->startGenerateSettings(m_pluginIdx);
    QTimer::singleShot(PROGRESS_DIALOG_DELAY_MS, this, &AlgorithmController::slot_showProgessDialog); // wait for 100ms before displaying the dialog
}

void AlgorithmController::slot_algorithmAborted()
{
    m_processRunning = false;
    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage(AlgorithmManager::instance().getPluginNameToIndex(m_pluginIdx) + tr(" aborted after ") + QString::number(duration_ms) + tr("ms"));
    if(m_progressDialogActivated){
        m_algorithmProgressDialog->close();
    }
}


void AlgorithmController::slot_previewStateChanged(bool enabled)
{
    emit sig_hasStatusMessage(enabled ? tr("Preview enabled.") : tr("Preview disabled."));
    TransformManager::instance().setTransformationEnabled(enabled);
}

void AlgorithmController::slot_updateKeyframes(QString pluginName, std::vector<uint> keyframes)
{
    QString activeName = AlgorithmManager::instance().getPluginNameToIndex(m_pluginIdx);
    if (pluginName == activeName) {
        //Update keyframes if change is sent by currently active plugin
        m_dataManager->getModelInputPictures()->updateMIP(keyframes);
        m_dataManager->getHistory()->slot_save();
        emit sig_keyframesChangedByPlugin(pluginName);
    }
}

void AlgorithmController::slot_updateBuffer(QString pluginName, QMap<QString, QVariant> buffer)
{

    QMapIterator<QString, QVariant> mapIter(buffer);
    while (mapIter.hasNext()) {
        mapIter.next();
        m_dataManager->getModelAlgorithm()->addPluginBuffer(pluginName, mapIter.key(), mapIter.value());
    }

}

void AlgorithmController::startNextTransform()
{
    m_future = m_fQueue;
    m_fQueue = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);
    emit sig_hasStatusMessage(tr("Computing preview ..."));
}

void AlgorithmController::slot_algorithmFinished(int)
{
    m_processRunning = false;
    auto duration_ms = m_timer.elapsed();
    if(m_progressDialogActivated){
        m_algorithmProgressDialog->close();
    }
    emit sig_hasStatusMessage(AlgorithmManager::instance().getPluginNameToIndex(m_pluginIdx) + tr(" finished after ") + QString::number(duration_ms) + tr("ms"));
    emit sig_algorithmFinished(m_pluginIdx);
    m_dataManager->getHistory()->slot_save();
}
