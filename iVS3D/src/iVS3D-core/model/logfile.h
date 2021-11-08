#ifndef LOGFILE_H
#define LOGFILE_H

#include "LogFileParent.h"

struct INTERNAL_PROCEDURE {
    QString name;
    qint64 elapsedTime;
    QString startTime;
    QString stopTime;
};

/**
 * @ingroup Model
 *
 * @brief The LogFile class logs progress as well as information about a process (e.g. Inport, Algorithm, Exoprt,...)
 *
 * @author Dominic Zahn
 *
 * @date 2021/06/24
 */
class LogFile : public LogFileParent
{
    Q_OBJECT
public:
    /**
     * @brief Default constructor
     * @param name of the logged process
     * @param isPlugin is true if the logged process is a plugin
     */
    LogFile(const QString &name, bool isPlugin);
    /**
     * @brief getName Returns the name
     * @return QString with the name
     */
    QString getName() override;
    /**
     * @brief getCreationTime Returns the time and date when the file was initalized
     * @return QString in form "dd.MM.yyyy h:m:s ap" which holds creation time and date
     */
    QString getCreationTime() override;
    /**
     * @brief getIsPlugin Returns true if the logged process is a plugin
     * @return bool with is true if the logged process is a plugin
     */
    bool getIsPlugin() override;
    /**
     * @brief setSettings Allows to log the used settings
     * @param settings Includes all used settings in form of a QMap<QString, QVariant>
     */
    void setSettings(QMap<QString, QVariant> settings) override;
    /**
     * @brief setInputInfo allows to log the used input
     * @param inputInfo is a QVariant which holds a summary of information about the result which is the process is generating
     */
    void setInputInfo(std::vector<uint> inputFrames) override;
    /**
     * @brief setResultsInfo allows to log the returned result of the process
     * @param resultInfo is a QVariant which holds a summary of information about the input which is given to the process
     */
    void setResultsInfo(std::vector<uint> keyframes) override;
    /**
     * @brief addCustomEntry allows to log with custom entries and types
     * @param entryName is a QString which identifies the entry
     * @param entryValue is a QVaraint that holds the custom value
     * @param type is an optional QString which could be used to classify custom entries
     */
    void addCustomEntry(QString entryName, QVariant entryValue, QString type = EMPTY_TYPE) override;
    /**
     * @brief startTimer Starts a timer to measure a specific internal process
     * @param timerName is a QString which identifies the internal process and is later displayed in the log file
     */
    void startTimer(QString timerName) override;
    /**
     * @brief stopTimer Stops the currently running timer and logs the surpassed timespan
     */
    void stopTimer() override;
    /**
     * @brief toQJSON Returns the current state of the log file in form of a QJsonObject
     * @return QJsonObject which holds all currently locked informationf from this process
     */
    QJsonObject toQJSON() override;
    /**
     * @brief print Saves the current sate of the log file as a file at the given path
     * @param path is the place where the log file will be saved
     * @return true if creation was successfull
     */
    bool print(QString path) override;

signals:
    void sig_updateLog();

private:
    QDateTime m_creationTime;
    bool m_isPlugin;
    QList<QJsonObject> m_customLog;
    QList<INTERNAL_PROCEDURE> m_procedureList;
    QElapsedTimer m_timer;
    QMap<QString, QVariant> m_settings;
    QJsonObject m_inputInfo, m_resultInfo;
    QString m_name;
    QString m_globalProcedureStart, m_globalProcedureStop;
    qint64 m_globalProcedureElapsed;
    QElapsedTimer m_totalProcedures;

    QVariant vectorToVariant(std::vector<uint> vector);
    QString getCurrentTime();
};

#endif // LOGFILE_H
