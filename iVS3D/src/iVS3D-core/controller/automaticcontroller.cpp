#include "automaticcontroller.h"

AutomaticController::AutomaticController(OutputWidget* outputWidget, AutomaticWidget *autoWidget, SamplingWidget *samplingWidget, DataManager* dm) :
    m_outputWidget(outputWidget), m_autoWidget(autoWidget), m_samplingWidget(samplingWidget)
{
    m_autoExecSettings = new AutomaticExecSettings();
    m_autoExec = new AutomaticExecutor(dm, m_autoExecSettings);
    //TODO Delete AutoExecSettings in Controller

    //Connect add signals
    connect(m_samplingWidget, &SamplingWidget::sig_addAuto, m_autoExecSettings, &AutomaticExecSettings::slot_addAuto);
    connect(m_outputWidget, &OutputWidget::sig_addAuto, m_autoExecSettings, &AutomaticExecSettings::slot_addAutoOutput);
    //Connect autoWidget signals
    connect(m_autoWidget, &AutomaticWidget::sig_saveConfiguration, this, &AutomaticController::slot_saveConfiguration);
    connect(m_autoWidget, &AutomaticWidget::sig_loadConfiguration, this, &AutomaticController::slot_loadConfiguration);

    connect(m_autoWidget, &AutomaticWidget::sig_startAutomaticExec, m_autoExec, &AutomaticExecutor::slot_startAutomaticExec);
    connect(m_autoWidget, &AutomaticWidget::sig_removedPlugin, m_autoExecSettings, &AutomaticExecSettings::slot_removedPlugin);
    connect(m_autoWidget, &AutomaticWidget::sig_doubleClickedItem, m_autoExecSettings, &AutomaticExecSettings::slot_doubleClickedItem);
    connect(m_autoWidget, &AutomaticWidget::sig_autoOrderChanged, m_autoExecSettings, &AutomaticExecSettings::slot_autoOrderChanged);
    //Connect signals from AutoExecSettings
    connect(m_autoExecSettings, &AutomaticExecSettings::sig_setAlgorithm, this, &AutomaticController::slot_setAlgorithm);
    connect(m_autoExecSettings, &AutomaticExecSettings::sig_updatedSelectedPlugins, this, &AutomaticController::slot_updatedSelectedPlugins);
    //Connect signals to AutoExecSettings
    connect(this, &AutomaticController::sig_saveConfiguration, m_autoExecSettings, &AutomaticExecSettings::slot_saveConfiguration);
    connect(this, &AutomaticController::sig_loadConfiguration, m_autoExecSettings, &AutomaticExecSettings::slot_loadConfiguration);
    //Connect signals from AutoExec
    connect(m_autoExec, &AutomaticExecutor::sig_createProgress, this, &AutomaticController::slot_createProgress);
    connect(m_autoExec, &AutomaticExecutor::sig_deleteProgress, this, &AutomaticController::slot_deleteProgress);


    // connects for displaying information
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        // ui is active
        if(m_autoWidget) {
            m_autoWidget->setEnabled(true);
        }
    }
}

void AutomaticController::slot_setAlgorithm(int index)
{
    m_samplingWidget->setAlgorithm(index);
}

void AutomaticController::slot_saveConfiguration()
{
    QString selectedFilter = "";
    QString savePath = QFileDialog::QFileDialog::getSaveFileName (m_autoWidget, "Save configuration", ApplicationSettings::instance().getStandardInputPath(), "*.json", &selectedFilter, QFileDialog::DontUseNativeDialog);
    if (savePath == nullptr) {
        return;
    }
    emit sig_saveConfiguration(savePath);
}

void AutomaticController::slot_loadConfiguration()
{
    QString selectedFilter = "";
    QString savePath = QFileDialog::getOpenFileName(m_autoWidget, "Choose configuration", ApplicationSettings::instance().getStandardInputPath(), "*.json", &selectedFilter, QFileDialog::DontUseNativeDialog);
    if (savePath == nullptr) {
        return;
    }
    emit sig_loadConfiguration(savePath);
}

void AutomaticController::slot_updatedSelectedPlugins(QStringList usedPlugins)
{
    m_autoWidget->updateSelectedPlugins(usedPlugins);
}

void AutomaticController::slot_createProgress(AlgorithmExecutor* algoExec)
{
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        m_algorithmProgressDialog = new ProgressDialog(m_autoWidget, true);
        connect(algoExec, &AlgorithmExecutor::sig_progress, m_algorithmProgressDialog, &ProgressDialog::slot_displayProgress);
        connect(algoExec, &AlgorithmExecutor::sig_message, m_algorithmProgressDialog, &ProgressDialog::slot_displayMessage);
        connect(m_algorithmProgressDialog, &ProgressDialog::sig_abort, algoExec, &AlgorithmExecutor::slot_abort);
        m_algorithmProgressDialog->setSizeGripEnabled(false);
        m_algorithmProgressDialog->show();
    } else {
        // terminal is active
        m_terminal = &TerminalInteraction::instance();
        connect(algoExec,&AlgorithmExecutor::sig_progress, m_terminal, &TerminalInteraction::slot_displayProgress);
        connect(algoExec, &AlgorithmExecutor::sig_message, m_terminal, &TerminalInteraction::slot_displayMessage);
    }
}

void AutomaticController::slot_deleteProgress()
{
    if (m_algorithmProgressDialog) {
       m_algorithmProgressDialog->close();
    }
    delete m_algorithmProgressDialog;
    m_algorithmProgressDialog = nullptr;
}

AutomaticExecutor *AutomaticController::autoExec()
{
    return m_autoExec;
}

void AutomaticController::disableAutoWidget()
{
    m_autoWidget->setEnabled(false);
}

AutomaticExecSettings *AutomaticController::autoExecSettings()
{
    return m_autoExecSettings;
}

void AutomaticController::setExporController(ExportController* exCon) {
    m_autoExecSettings->setExportController(exCon);
    m_autoExec->setExportController(exCon);
    connect(m_autoExecSettings, &AutomaticExecSettings::sig_showExportSettings, exCon, &ExportController::slot_showExportSettings);
    m_autoWidget->setEnabled(true);
}
