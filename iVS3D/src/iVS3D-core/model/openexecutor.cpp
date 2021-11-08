#include "openexecutor.h"

OpenExecutor::OpenExecutor(const QString &pathToOpen, DataManager *dataManager) : m_futurePicsWatcher(nullptr), m_dataManager(dataManager), m_path(pathToOpen)
{

}

OpenExecutor::~OpenExecutor()
{
    if(m_futurePicsWatcher){
        disconnect(m_futurePicsWatcher, &QFutureWatcher<int>::finished, this, &OpenExecutor::slot_finished);
        delete m_futurePicsWatcher;
    }
}

void OpenExecutor::open()
{

    if(isProjectPath()){
        m_futurePics = QtConcurrent::run(m_dataManager, &DataManager::openProject, m_path);
    } else {
        m_futurePics = QtConcurrent::run(m_dataManager, &DataManager::open, m_path);
    }

    if(m_futurePicsWatcher){
        disconnect(m_futurePicsWatcher, &QFutureWatcher<int>::finished, this, &OpenExecutor::slot_finished);
        delete m_futurePicsWatcher;
    }
    m_futurePicsWatcher = new QFutureWatcher<int>(this);
    connect(m_futurePicsWatcher, &QFutureWatcher<int>::finished, this, &OpenExecutor::slot_finished);
    m_futurePicsWatcher->setFuture(m_futurePics);
}

void OpenExecutor::slot_finished()
{
    emit sig_finished(m_futurePicsWatcher->result());
}

bool OpenExecutor::isProjectPath()
{
    return m_path.endsWith(".json");
}
