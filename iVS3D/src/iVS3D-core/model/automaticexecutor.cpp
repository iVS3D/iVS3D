#include "automaticexecutor.h"


AutomaticExecutor::AutomaticExecutor(DataManager* dm, AutomaticWidget* autoWidget, AutomaticExecSettings* autoSettings, ExportController* exportController)
    :m_dm(dm), m_autoWidget(autoWidget), m_autoSettings(autoSettings), m_exportController(exportController)

{
    m_algoExec = new AlgorithmExecutor(m_dm);
    m_algorithmProgressDialog = nullptr;
    // only ui
    connect(autoWidget, &AutomaticWidget::sig_startAutomaticExec, this, &AutomaticExecutor::slot_startAutomaticExec);

    // prevents conflicts with concurrent access from ui
    emit sig_stopPlay();

    // connects for displaying information
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        // ui is active
        if(m_autoWidget) {
            m_autoWidget->setEnabled(true);
        }
    }
}

AutomaticExecutor::~AutomaticExecutor()
{
    if(m_autoWidget) {
        m_autoWidget->setEnabled(false);
    }
}

void AutomaticExecutor::startMultipleAlgo(QList<QPair<QString, QMap<QString, QVariant>>> algoList, int step)
{
    //Get all plugins
    if (algoList[step].first.compare(stringContainer::Export) == 0) {
        executeExport(algoList[step].second);
        return;
    }

    int idx = stepToPluginIndex(step);

    //Check if index is valid
    if (idx == -1) {
        emit sig_hasStatusMessage("Plugin " + algoList[step].first + " not found");
        m_step++;
        slot_samplingFinished();
        return;
    }

    //Set settings of the current plugin
    QMap<QString, QVariant> settings = algoList[step].second;
    if (settings.isEmpty()) {
        connect(m_algoExec, &AlgorithmExecutor::sig_pluginFinished, this, &AutomaticExecutor::slot_settingsFinished, Qt::DirectConnection);
        m_algoExec->startGenerateSettings(idx);
    }
    else {
        // settings already exist so the sampling can be directly started
        AlgorithmManager::instance().setSettings(idx, m_pluginOrder[m_step].second);
        executeSampling();
    }
}

bool AutomaticExecutor::isFinished()
{
    return m_isFinished;
}

void AutomaticExecutor::slot_startAutomaticExec()
{
    m_step = 0;
    m_isFinished = false;
    m_pluginOrder = m_autoSettings->getPluginList();
    m_stepCount = m_pluginOrder.size();
    if (m_stepCount != 0) {
        m_algoExec = new AlgorithmExecutor(m_dm);
        if (qApp->property(stringContainer::UIIdentifier).toBool()) {
            m_algorithmProgressDialog = new ProgressDialog(m_autoWidget, true);
            connect(m_algoExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);
            connect(m_algoExec, &AlgorithmExecutor::sig_message, m_algorithmProgressDialog, &ProgressDialog::slot_displayMessage);
            connect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, m_algoExec, &AlgorithmExecutor::slot_abort);
            m_algorithmProgressDialog->setSizeGripEnabled(false);
            m_algorithmProgressDialog->show();
        } else {
            // terminal is active
            m_terminal = &TerminalInteraction::instance();
            connect(m_algoExec,&AlgorithmExecutor::sig_progress, m_terminal, &TerminalInteraction::slot_displayProgress);
            connect(m_algoExec, &AlgorithmExecutor::sig_message, m_terminal, &TerminalInteraction::slot_displayMessage);
        }
        startMultipleAlgo(m_pluginOrder, 0);
    }

}


void AutomaticExecutor::slot_samplingFinished()
{
    // disconnect so that for example generateSettings can use them
    QObject::disconnect(m_algoExec, &AlgorithmExecutor::sig_pluginFinished, this, &AutomaticExecutor::slot_samplingFinished);
    QObject::disconnect(m_algoExec, &AlgorithmExecutor::sig_algorithmAborted, this, &AutomaticExecutor::slot_algoAbort);

    if (m_step < m_stepCount) {
       startMultipleAlgo(m_pluginOrder, m_step);
    }
    else {
        // cleanup because every step of the batch processing is down now
        emit sig_hasStatusMessage("Finished " + QString::number(m_stepCount) + " sampling algorithms");
        if (m_algorithmProgressDialog) {
           m_algorithmProgressDialog->close();
        }
        //Reset the algoExecutor and the progressDialog
        if (m_algoExec) {
            delete m_algoExec;
            m_algoExec = nullptr;
        }
        disconnect(m_exportController, &ExportController::sig_exportFinished, this, &AutomaticExecutor::slot_samplingFinished);
        delete m_algorithmProgressDialog;
        m_algorithmProgressDialog = nullptr;
        m_step = 0;
        m_isFinished = true;

    }
}

void AutomaticExecutor::slot_algoAbort()
{
    emit sig_hasStatusMessage("Aborted batch processing");
    if (m_algorithmProgressDialog) {
       m_algorithmProgressDialog->close();
    }
    m_step = 0;
}

void AutomaticExecutor::slot_settingsFinished()
{
    // disconnect to keep the signal free for the following sampling process
    QObject::disconnect(m_algoExec, &AlgorithmExecutor::sig_pluginFinished, this, &AutomaticExecutor::slot_settingsFinished);

    executeSampling();
}

int AutomaticExecutor::stepToPluginIndex(int step)
{
    QStringList pluginList = AlgorithmManager::instance().getAlgorithmList();
    return pluginList.indexOf(m_pluginOrder[step].first);
}

void AutomaticExecutor::executeSampling()
{
    // connecting the AlgorithmExecuter for sampling usage
    connect(m_algoExec, &AlgorithmExecutor::sig_pluginFinished, this, &AutomaticExecutor::slot_samplingFinished, Qt::DirectConnection);
    connect(m_algoExec, &AlgorithmExecutor::sig_algorithmAborted, this, &AutomaticExecutor::slot_algoAbort);

    //start the plugin using keyframes, only the first plugin uses all images
    int idx = stepToPluginIndex(m_step);
    if(m_step == 0) {
      m_algoExec->startSampling(idx, false, false);
    }
    else {
      m_algoExec->startSampling(idx, true, false);
    }

    m_step++;
}

void AutomaticExecutor::executeExport(QMap<QString, QVariant> settings)
{
    //dont use ui, if command line is active
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        if (m_algorithmProgressDialog) {
           m_algorithmProgressDialog->close();
        }
        m_exportController->setOutputSettings(settings);
        connect(m_exportController, &ExportController::sig_exportFinished, this, &AutomaticExecutor::slot_samplingFinished);
        m_exportController->slot_export();
    }
    else {
        if (m_exportRunner) {
            delete m_exportRunner;
        }
        m_exportRunner = new noUIExport(settings, m_dm);
        connect(m_exportRunner, &noUIExport::sig_exportFinished, this, &AutomaticExecutor::slot_samplingFinished, Qt::DirectConnection);
        m_exportRunner->runExport();
    }
    m_step++;
}



