#ifndef LOGFILEPARENT_H
#define LOGFILEPARENT_H

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QVariant>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <sstream>

#include "stringcontainer.h"

#define EMPTY_TYPE ""
#define DATE_FORMAT "dd.MM.yyyy hh:mm:ss.z"

/**
 * @ingroup Model
 *
 * @brief The LogFileParent class logs progress as well as information about a process (e.g. Inport, Algorithm, Exoprt,...)
 *
 * @author Dominic Zahn
 *
 * @date 2021/06/24
 */
class LogFileParent : public QObject
{
    //Q_OBJECT
public:
    /**
     * @brief getName Returns the name
     * @return QString with the name
     */
    virtual QString getName() = 0;
    /**
     * @brief getCreationTime Returns the time and date when the file was initalized
     * @return QString in form "dd.MM.yyyy h:m:s ap" which holds creation time and date
     */
    virtual QString getCreationTime() = 0;
    /**
     * @brief getIsPlugin Returns true if the logged process is a plugin
     * @return bool with is true if the logged process is a plugin
     */
    virtual bool getIsPlugin() = 0;
    /**
     * @brief setSettings Allows to log the used settings
     * @param settings Includes all used settings in form of a QMap<QString, QVariant>
     */
    virtual void setSettings(QMap<QString, QVariant> settings) = 0;
    /**
     * @brief setInputInfo allows to log the used input
     * @param inputInfo is a QVariant which holds a summary of information about the result which is the process is generating
     */
    virtual void setInputInfo(std::vector<uint> inputFrames) = 0;
    /**
     * @brief setResultsInfo allows to log the returned result of the process
     * @param resultInfo is a QVariant which holds a summary of information about the input which is given to the process
     */
    virtual void setResultsInfo(std::vector<uint> keyframes) = 0;
    /**
     * @brief addCustomEntry allows to log with custom entries and types
     * @param entryName is a QString which identifies the entry
     * @param entryValue is a QVaraint that holds the custom value
     * @param type is an optional QString which could be used to classify custom entries
     */
    virtual void addCustomEntry(QString entryName, QVariant entryValue, QString type = EMPTY_TYPE) = 0;
    /**
     * @brief startTimer Starts a timer to measure a specific internal process
     * @param timerName is a QString which identifies the internal process and is later displayed in the log file
     */
    virtual void startTimer(QString timerName) = 0;
    /**
     * @brief stopTimer Stops the currently running timer and logs the surpassed timespan
     */
    virtual void stopTimer() = 0;
    /**
     * @brief toQJSON Returns the current state of the log file in form of a QJsonObject
     * @return QJsonObject which holds all currently locked informationf from this process
     */
    virtual QJsonObject toQJSON() = 0;
    /**
     * @brief print Saves the current sate of the log file as a file at the given path
     * @param path is the place where the log file will be saved
     * @return true if creation was successfull
     */
    virtual bool print(QString path) = 0;
};

#endif // LOGFILEPARENT_H
