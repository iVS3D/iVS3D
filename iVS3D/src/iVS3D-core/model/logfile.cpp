#include "logfile.h"

LogFile::LogFile(const QString &name, bool isPlugin) :
   m_creationTime(QDateTime::currentDateTime()), m_procedureList({}), m_timer(QElapsedTimer()), m_settings({}),
   m_inputInfo({}), m_resultInfo({}), m_name(name), m_globalProcedureStart(""), m_globalProcedureStop("")
{
    m_globalProcedureElapsed = 0;
    m_isPlugin = isPlugin;

}

QString LogFile::getName()
{
    return m_name;
}

QString LogFile::getCreationTime()
{
    return m_creationTime.toString(DATE_FORMAT);
}

bool LogFile::getIsPlugin()
{
    return m_isPlugin;
}

void LogFile::setSettings(QMap<QString, QVariant> settings)
{
    m_settings = settings;
    emit sig_updateLog();
}

void LogFile::setInputInfo(std::vector<uint> inputFrames)
{
    QVariant varInpuFrames = vectorToVariant(inputFrames);
    m_inputInfo.insert(stringContainer::logKeyframesIdentifier, QJsonValue::fromVariant(varInpuFrames));
    m_inputInfo.insert(stringContainer::logKeyframeCountIdentifier, (int)inputFrames.size());
    emit sig_updateLog();
}

void LogFile::setResultsInfo(std::vector<uint> keyframes)
{
    QVariant varKeyframes = vectorToVariant(keyframes);
    m_resultInfo.insert(stringContainer::logKeyframesIdentifier, QJsonValue::fromVariant(varKeyframes));
    m_resultInfo.insert(stringContainer::logKeyframeCountIdentifier, (int)keyframes.size());
    emit sig_updateLog();
}

void LogFile::addCustomEntry(QString entryName, QVariant entryValue, QString type)
{
    QJsonObject jsonEntry;
    // set type if it was set
    if (type != EMPTY_TYPE) {
        jsonEntry.insert(stringContainer::logTypeIdentifier, type);
    }
    // set entry value
    jsonEntry.insert(entryName, QJsonValue::fromVariant(entryValue));
    m_customLog.push_back(jsonEntry);
    emit sig_updateLog();
}

void LogFile::startTimer(QString timerName)
{    
    // check if timer is already running and stop timer appropriate if neccessary
    if (m_timer.isValid()) {
        stopTimer();
    }

    QString startTime = getCurrentTime();
    // log global start time if this is the first time starteTimer was called
    if (m_procedureList.isEmpty()) {
        m_globalProcedureStart = startTime;
        m_totalProcedures.start();
    }

    // create new empty list object to later fill in elapsed time
    INTERNAL_PROCEDURE nInternalProcedure = {timerName, 0, startTime, ""};
    m_procedureList << nInternalProcedure;
    m_timer.start();
}

void LogFile::stopTimer()
{
    // abort if timer is not running
    if (!m_timer.isValid() || m_procedureList.isEmpty()) {
        return;
    }

    // read out and stop timer
    qint64 elapsedTime = m_timer.elapsed();
    m_timer.invalidate();

    // fill elapsed time in already created list object
    int activeProcedure = m_procedureList.size() - 1;
    m_procedureList[activeProcedure].elapsedTime = elapsedTime;
    m_procedureList[activeProcedure].stopTime = getCurrentTime();

    // update global end timer
    m_globalProcedureElapsed = m_totalProcedures.elapsed();
    m_globalProcedureStop = getCurrentTime();
    emit sig_updateLog();
}

QJsonObject LogFile::toQJSON()
{
    QJsonObject jsonAll;
    // input
    QJsonObject jsonInput = m_inputInfo;
    // settings
    QJsonObject jsonSettings = QJsonObject::fromVariantMap(m_settings);
    // total procedure info
    QJsonObject jsonTotalProcedure;
    jsonTotalProcedure.insert(stringContainer::logElapsedTimeIdentifier, m_globalProcedureElapsed);
    jsonTotalProcedure.insert(stringContainer::logStartTimeIdentifier, m_globalProcedureStart);
    jsonTotalProcedure.insert(stringContainer::logStopTimeIdentifier, m_globalProcedureStop);
    // procedure (logged times)
    QJsonArray jsonProcedure;
    //      add internal procedures
    for (INTERNAL_PROCEDURE currProcedure : m_procedureList) {
        QJsonObject jsonProcedureEntry;
        jsonProcedureEntry.insert(stringContainer::logNameIdentifier, currProcedure.name);
        jsonProcedureEntry.insert(stringContainer::logElapsedTimeIdentifier, currProcedure.elapsedTime);
        jsonProcedureEntry.insert(stringContainer::logStartTimeIdentifier, currProcedure.startTime);
        jsonProcedureEntry.insert(stringContainer::logStopTimeIdentifier, currProcedure.stopTime);
        jsonProcedure.push_back(jsonProcedureEntry);
    }
    // custom log area
    QJsonArray jsonCustom;
    for (QJsonObject currCustomEntry : m_customLog) {
        QString logTime = QDateTime::currentDateTime().toString(DATE_FORMAT);
        currCustomEntry.insert(stringContainer::logCreationTimeIdentifier, logTime);
        jsonCustom.push_back(currCustomEntry);
    }
    // results
    QJsonObject jsonResults = m_resultInfo;

    // put all parts of the current log together
    jsonAll.insert(stringContainer::logNameIdentifier, m_name);
    jsonAll.insert(stringContainer::logCreationTimeIdentifier, getCreationTime());
    jsonAll.insert(stringContainer::logInputIdentifier, jsonInput);
    jsonAll.insert(stringContainer::logSettingsIdentifier, jsonSettings);
    jsonAll.insert(stringContainer::logTotalProcedureIdentifier, jsonTotalProcedure);
    jsonAll.insert(stringContainer::logProcedureIdentifier, jsonProcedure);
    jsonAll.insert(stringContainer::logCustomIdentifier, jsonCustom);
    jsonAll.insert(stringContainer::logResultsIdentifier, jsonResults);
    jsonAll.insert(stringContainer::logIsPluginIdentifier, m_isPlugin);

    return jsonAll;
}

bool LogFile::print(QString path)
{
    QFile file(path);
    QJsonDocument doc(toQJSON());

    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        return false;
    }
    file.write(doc.toJson());
    file.close();
    return true;
}

QVariant LogFile::vectorToVariant(std::vector<uint> vector)
{
    //Keyframe vector to Variant
    std::stringstream sStream;
    for (uint i = 0; i < vector.size(); i++) {
        if (i != 0) {
           sStream << stringContainer::jsonDelimiter.toStdString();
        }
        sStream << vector[i];
    }
    std::string key = sStream.str();
    QVariant varVector(QString::fromStdString(key));
    return varVector;
}

QString LogFile::getCurrentTime()
{
    return QDateTime::currentDateTime().toString(DATE_FORMAT);
}
