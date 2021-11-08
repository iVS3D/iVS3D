#ifndef LOGFILE_STUB_H
#define LOGFILE_STUB_H

#include <LogFileParent.h>

class LogFile_stub : public LogFileParent
{

public:
    LogFile_stub(QString name, bool isPlugin);

    QString getName() { throw "not implemented"; };

    QString getCreationTime() { throw "not implemented"; };

    bool getIsPlugin() { throw "not implemented"; };

    void setSettings(QMap<QString, QVariant> settings) { return; };

    void setInputInfo(std::vector<uint> inputFrames) { return; };

    void setResultsInfo(std::vector<uint> keyframes) { return; };

    void addCustomEntry(QString entryName, QVariant entryValue, QString type = EMPTY_TYPE)  { return; };

    void startTimer(QString timerName);

    void stopTimer() { return; };

    QJsonObject toQJSON() { throw "not implemented"; };

    bool print(QString path) { throw "not implemented"; };

};

#endif // LOGFILE_STUB_H
