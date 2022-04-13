#ifndef SIGNALOBJECT_H
#define SIGNALOBJECT_H

#include <QObject>

class signalObject : public QObject
{
    Q_OBJECT
public:
    signalObject(QObject *parent = nullptr);
    virtual void newMetaData();

signals:
    void sig_newMetaData();

};

#endif // SIGNALOBJECT_H
