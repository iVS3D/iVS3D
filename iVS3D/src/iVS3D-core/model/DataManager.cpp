#include "DataManager.h"

DataManager::DataManager()
{
    m_projectManager = new ProjectManager();
    m_ma = nullptr;
    m_mip = nullptr;
}

DataManager::~DataManager()
{
    delete m_projectManager;
    if(m_mip){
        delete m_mip;
    }
    if (m_ma) {
        delete m_ma;
    }
}

int DataManager::open(QString path)
{
    LogManager::instance().resetLog();
    delete m_projectManager;
    m_projectManager = new ProjectManager;
    if(m_mip){
        delete m_mip;
    }

    LogFile *lf = LogManager::instance().createLogFile(stringContainer::lfImportProcess, false);
    lf->startTimer(stringContainer::lfImportMip);
    m_mip = new ModelInputPictures(path);
    lf->stopTimer();
    QMap<QString, QVariant> settingsInput;
    settingsInput.insert(stringContainer::lfImportPath, path);
    lf->setSettings(settingsInput);

    m_ma = new ModelAlgorithm;
    return m_mip->getPicCount();
}

int DataManager::openProject(QString path)
{
    LogManager::instance().resetLog();
    delete m_projectManager;
    m_projectManager = new ProjectManager;
    if(m_mip){
        delete m_mip;
    }
    if(m_ma){
        delete m_ma;
    }

    m_mip = new ModelInputPictures();
    m_ma = new ModelAlgorithm();

    LogFile *lf = LogManager::instance().createLogFile(stringContainer::lfImportProcess, false);
    lf->startTimer(stringContainer::lfImportProject);
    bool success = m_projectManager->loadProject(m_mip, m_ma, path);
    lf->stopTimer();
    QMap<QString, QVariant> settingsInput;
    settingsInput.insert(stringContainer::lfImportProjectPath, path);
    lf->setSettings(settingsInput);

    if (success)
    {
        return m_mip->getPicCount();
    }
    else
    {
        return 0;
    }
}

ModelAlgorithm* DataManager::getModelAlgorithm()
{
    return m_ma;
}

ModelInputPictures* DataManager::getModelInputPictures()
{
    return m_mip;
}


void DataManager::saveProject()
{
    m_projectManager->saveProject(m_mip, m_ma);
}

void DataManager::saveProjectAs(QString projectName, QString projectPath)
{
    m_projectManager->saveProjectAs(m_mip, m_ma, projectPath, projectName);
}

void DataManager::createProject(QString projectName, QString projectPath)
{
    m_projectManager->createProject(m_mip, m_ma, projectPath, projectName);
}

bool DataManager::isProjectLoaded()
{
    return m_projectManager->isProjectLoaded();
}

QString DataManager::getProjectName()
{
    return m_projectManager->getProjectName();
}

QString DataManager::getProjectPath()
{
    return m_projectManager->getProjectPath();
}


