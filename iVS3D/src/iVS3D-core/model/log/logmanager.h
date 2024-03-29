#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QList>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

#include "stringcontainer.h"
#include "logfile.h"

#define FILE_NAME_SUFFIX "_ddMMyyyy_hhmmss"
#define FULL_FILE_PATH m_logDir + (QString)"/" + m_fileName + (QString)".json"

/**
 * @class LogManager
 *
 * @ingroup Model
 *
 * @brief The LogManager class is a singleton which manages the logging process.
 *        It provides LogFiles that are used to log information which belongs to a single process.
 *
 * @author Dominic Zahn
 *
 * @date 2021/07/06
 */
class LogManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Returns the singleton instance of this class
     * @return Instance of this class
     */
    static LogManager& instance();
    /**
     * @brief createLogFile Returns the pointer to an already existing LogFile or creates a new one if neccessary.
     *          LogFiles are used to log a single process. (e.g. Import, Export, Nth-Frame,...)
     * @param name of the returned LogFile
     * @return Pointer to a new LogFile or an already existing one with the given name
     */
    LogFile* createLogFile(QString name, bool isPlugin);
    /**
     * @brief deleteAllLogFiles Deletes all LogFiles. Every existing pointer is invalid now.
     */
    void deleteAllLogFiles();
    /**
     * @brief toQJSON Returns the current state of the log files in form of a QJsonObject
     * @return QJsonObject which holds all currently logged informationf from all process (all LogFiles)
     */
    QJsonDocument toJSON();

    /**
     * @brief resetLog resets the current log and creates a new log
     */
    void resetLog();
    /**
     * @brief toggleLog is used to enable or disable loging (-> saving the log file to disk or not)
     *
     * @param useLog @a true when log is enabled, @a false otherwise
     */
    void toggleLog(bool useLog);
    /**
     * @brief setLogDirectory changes the directory where the log file is printed to.
     *        It wont an artifical folder in between and just use the raw path instead.
     *
     * @param Path to the new directory where log files will be stored.
     */
    void setLogDirectory(QString logDir);

public slots:

    /**
     * @brief update is called whenever a logfile changes. This will print the current log files.
     * @return true if creation was successfull
     */
    bool slot_updateLog();

private:
    bool print();
    LogManager();
    QList<QPair<QString, LogFile*>> m_allLogFiles;
    QString m_fileName;
    QString m_logDir;
    bool m_logEnabled = true;
};

#endif // LOGMANAGER_H
