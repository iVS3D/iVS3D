#ifndef STACKCONTROLLER_H
#define STACKCONTROLLER_H

#include <QObject>
#include "history.h"
#include "operationstack.h"



class StackController : public QObject
{
    Q_OBJECT

public:
    StackController(OperationStack* opStack, History* mipHistory);


private:
    OperationStack* m_opStack;
    History* m_history;
    void deleteInvalidFuture();


public slots:
    void slot_toggleKeyframe(uint idx, bool isNowKeyframe);
    void slot_deleteKeyframes();
    void slot_deleteAllKeyframes();
    void slot_rowClicked(int row);
    void slot_algorithmFinished(int index);
    void slot_keyframesChangedByPlugin(QString pluginName);
};

#endif // STACKCONTROLLER_H
