#include "exportexecutor.h"

ExportExecutor::ExportExecutor(QObject* parent, DataManager* dataManager)
{
    m_parent = parent;
    m_dataManager = dataManager;
    m_exportThread = nullptr;

    m_boundaries = m_dataManager->getModelInputPictures()->getBoundaries();
}

void ExportExecutor::startExport(const ExportConfig& config, LogFile *logFile){

    ModelInputPictures* mip = m_dataManager->getModelInputPictures();
    // cause mip loses its boundary attribute in a magical and unkown way
    mip->setBoundaries(m_boundaries);
    m_exportThread = new ExportThread(this, mip, config, &m_stopped, logFile);
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
