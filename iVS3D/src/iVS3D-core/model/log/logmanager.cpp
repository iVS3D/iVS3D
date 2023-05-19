#include "logmanager.h"

LogManager::LogManager() : m_allLogFiles({})
{
    //      create unique name to path
    m_fileName = "log" + QDateTime::currentDateTime().toString(FILE_NAME_SUFFIX);
    //      set default log path
    m_logDir = QCoreApplication::applicationDirPath() + "/logs";
}

LogManager &LogManager::instance()
{
    static LogManager INSTANCE;
    return INSTANCE;
}

LogFile *LogManager::createLogFile(QString name, bool isPlugin)
{
    // create new logFile and add it to map
    LogFile *nLogFile = new LogFile(name, isPlugin);
    m_allLogFiles.push_back(QPair<QString, LogFile*>(name, nLogFile));
    connect(nLogFile, &LogFile::sig_updateLog, this, &LogManager::slot_updateLog, Qt::DirectConnection);
    return nLogFile;
}

void LogManager::deleteAllLogFiles()
{
    for (int i = 0; i < m_allLogFiles.size(); i++) {
        QPair<QString, LogFile*> entry = m_allLogFiles[i];
        // delete LogFile and remove it from list
        disconnect(entry.second, &LogFile::sig_updateLog, this, &LogManager::slot_updateLog);
        delete entry.second;
        m_allLogFiles.removeAt(i);
    }
    m_allLogFiles.clear();
}

QJsonDocument LogManager::toJSON()
{
    QJsonArray arrayAll;
    // gather all LogFiles as QJsons in an array
    for (QPair<QString, LogFile*> entry : m_allLogFiles) {
        LogFile* lf = entry.second;
        arrayAll.push_back(lf->toQJSON());
    }

    QJsonDocument jsonDoc;
    jsonDoc.setArray(arrayAll);
    return  jsonDoc;
}

void LogManager::resetLog()
{
    deleteAllLogFiles();
    slot_updateLog();
}

void LogManager::toggleLog(bool useLog)
{
    m_logEnabled = useLog;
}

void LogManager::setLogDirectory(QString logDir)
{
    m_logDir = logDir;
}

bool LogManager::slot_updateLog()
{
    if (!m_logEnabled) {
        return false;
    }

    // create neccessary direcotries
    if (!QDir(m_logDir).exists()) {
        QDir().mkpath(FULL_FILE_PATH);
    }

    // writing json in file
    QFile file(FULL_FILE_PATH);
    QJsonDocument doc(toJSON());

    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return false;
    }
    file.write(doc.toJson());
    file.close();
    return true;
}

