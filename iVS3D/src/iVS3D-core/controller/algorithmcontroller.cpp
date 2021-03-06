#include "algorithmcontroller.h"


AlgorithmController::AlgorithmController(DataManager* dataManager, SamplingWidget *samplingWidget)
{
    m_dataManager = dataManager;
    m_pluginIdx = samplingWidget->getSelectedAlgorithm();
    m_pluginType = (samplingWidget->getSelctedType() ? PluginType::Algorithm : PluginType::Transform);
    AlgorithmManager::instance().initializePlugins(m_dataManager->getModelInputPictures()->getReader());
    samplingWidget->resetSelectedImages();
    //Save samplingSettings and connect
    m_samplingWidget = samplingWidget;
    m_samplingWidget->setEnabled(true);
    connect(m_samplingWidget, &SamplingWidget::sig_selectedAlgorithmChanged, this, &AlgorithmController::slot_selectAlgorithm);
    connect(m_samplingWidget, &SamplingWidget::sig_selectedTransformChanged, this, &AlgorithmController::slot_selectTransform);
    connect(m_samplingWidget, &SamplingWidget::sig_startSampling, this, &AlgorithmController::slot_startAlgorithm);
    connect(m_samplingWidget, &SamplingWidget::sig_enablePreviewChanged, this, &AlgorithmController::slot_previewStateChanged);
    //Create new AlgorithmExecutor
    m_algExec = new AlgorithmExecutor(m_dataManager);
    connect(m_algExec, &AlgorithmExecutor::sig_pluginFinished, this, &AlgorithmController::slot_algorithmFinished);
    connect(m_algExec, &AlgorithmExecutor::sig_algorithmAborted, this, &AlgorithmController::slot_algorithmAborted);

    m_algorithmProgressDialog = nullptr;

    m_future = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);
    m_fQueue = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);
}

AlgorithmController::~AlgorithmController()
{
    m_samplingWidget->setEnabled(false);
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

    emit sig_hasStatusMessage("Selected algorithm: " + AlgorithmManager::instance().getAlgorithmList()[idx]);
    TransformManager::instance().selectTransform(UINT_MAX);
}

void AlgorithmController::slot_selectTransform(int idx)
{
    m_pluginIdx = idx;
    m_pluginType = PluginType::Transform;

    m_samplingWidget->showAlgorithmSettings(TransformManager::instance().getSettingsWidget(m_samplingWidget, idx));

    emit sig_hasStatusMessage("Selected transformation: " + TransformManager::instance().getTransformList()[idx]);
    TransformManager::instance().selectTransform(idx);
}

void AlgorithmController::slot_startAlgorithm(bool onlyKeyframes, bool useBounds)
{
    emit sig_stopPlay();
    // upadate mip (boundaries)
    emit sig_updateBoundaries();

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

    // display progress dialog and start algorithm
    m_timer = QElapsedTimer();
    m_timer.start();
    m_algorithmProgressDialog->show();
    m_algExec->startSampling(m_pluginIdx, onlyKeyframes, useBounds);
}

void AlgorithmController::slot_algorithmAborted()
{
    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage(AlgorithmManager::instance().getAlgorithmList()[m_pluginIdx] + " aborted after " + QString::number(duration_ms) + "ms");
    m_algorithmProgressDialog->close();
}


void AlgorithmController::slot_previewStateChanged(bool enabled)
{
    emit sig_hasStatusMessage(enabled ? "Preview enabled." : "Preview disabled.");
    TransformManager::instance().setTransformationEnabled(enabled);
}

void AlgorithmController::startNextTransform()
{
    m_future = m_fQueue;
    m_fQueue = std::make_tuple<cv::Mat*, int, int>(nullptr, NO_IMAGE, NO_TRANSFORM);
    emit sig_hasStatusMessage("Computing preview ...");
}

void AlgorithmController::slot_algorithmFinished()
{
    auto duration_ms = m_timer.elapsed();
    emit sig_hasStatusMessage(AlgorithmManager::instance().getAlgorithmList()[m_pluginIdx] + " finished after " + QString::number(duration_ms) + "ms");
    m_algorithmProgressDialog->close();
}
