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

    void setSettings(QMap<QString, QVariant>) { throw "not implemented"; };

    void setInputInfo(std::vector<uint>) { throw "not implemented"; };

    void setResultsInfo(std::vector<uint>) { throw "not implemented"; };

    void addCustomEntry(QString, QVariant, QString = EMPTY_TYPE) { throw "not implemented"; };

    void startTimer(QString) { };

    void stopTimer() { };

    QJsonObject toQJSON() { throw "not implemented"; };

    bool print(QString) { throw "not implemented"; };

};


#endif // LOGFILE_STUB_H
