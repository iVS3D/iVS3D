#include "exportexecutor.h"

ExportExecutor::ExportExecutor(QObject* parent, DataManager* dataManager, std::shared_ptr<PluginThread> pluginThread)
{
    m_parent = parent;
    m_dataManager = dataManager;
    m_pluginThread = pluginThread;
    m_exportThread = nullptr;

    m_boundaries = m_dataManager->getModelInputPictures()->getBoundaries();
}

void ExportExecutor::startExport(const ExportConfig& config, std::shared_ptr<LogFile> logFile){
    m_stopped = false;

    const auto validation =
        ExportThread::validateMaskStack(config, m_pluginThread.get());
    if (!validation) {
        slot_displayMessage(validation.error().message);
        if (logFile) {
            logFile->addCustomEntry("MaskStackValidation",
                                    validation.error().message,
                                    "Error");
        }
        emit sig_exportFinished(ExportResult::failed(validation.error().message));
        return;
    }

    for (const QString& warning : validation->warnings) {
        emit sig_warning(warning);
        if (logFile) {
            logFile->addCustomEntry("MaskStackValidation", warning,
                                    "Warning");
        }
    }

    ModelInputPictures* mip = m_dataManager->getModelInputPictures();
    // cause mip loses its boundary attribute in a magical and unkown way
    mip->setBoundaries(m_boundaries);
    m_exportThread = new ExportThread(this, mip, config, &m_stopped, logFile, m_pluginThread.get());
    if (qApp->property(stringContainer::UIIdentifier).toBool()) {
        connect(m_exportThread, &ExportThread::finished, this, &ExportExecutor::slot_finished);
    }
    else {
        connect(m_exportThread, &ExportThread::finished, this, &ExportExecutor::slot_finished, Qt::DirectConnection);
    }
    m_exportThread->start();
    emit sig_exportStarted();
}

void ExportExecutor::slot_abort(){
    if(!m_exportThread){
        return;
    }
    slot_makeProgress(0, tr("Abort requested..."));
    slot_displayMessage(tr("Abort requested, stopping export..."));
    disconnect(m_exportThread, &ExportThread::finished, this, &ExportExecutor::slot_finished);
    m_stopped = true;
    m_exportThread->wait();
    closeThread();
    emit sig_exportAborted();
}

void ExportExecutor::slot_finished(){
    auto result = m_exportThread->getResult();
    closeThread();
    emit sig_exportFinished(result);
}

void ExportExecutor::closeThread(){
    disconnect(m_exportThread, &ExportThread::finished, this, &ExportExecutor::slot_finished);
    m_exportThread->deleteLater();
    //delete m_exportThread;
    m_exportThread = nullptr;
}
