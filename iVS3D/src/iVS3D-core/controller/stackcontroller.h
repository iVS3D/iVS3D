#ifndef STACKCONTROLLER_H
#define STACKCONTROLLER_H

#include <QObject>
#include "history.h"
#include "operationstack.h"
#include "samplingwidget.h"


class StackController : public QObject
{
    Q_OBJECT

public:
    StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget);
    void select();


private:
    OperationStack* m_opStack;
    History* m_history;
    SamplingWidget* m_samplingWidget;
    void deleteInvalidFuture();
    QMap<QString, QPair<int, QMap<QString, QVariant>>> m_algoSettings;


public slots:
    void slot_toggleKeyframe(uint idx, bool isNowKeyframe);
    void slot_deleteKeyframes();
    void slot_deleteAllKeyframes();
    void slot_rowClicked(int row);
    void slot_algorithmFinished(int index);
    void slot_keyframesChangedByPlugin(QString pluginName);
};

#endif // STACKCONTROLLER_H
