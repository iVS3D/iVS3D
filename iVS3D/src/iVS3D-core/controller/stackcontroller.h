#ifndef STACKCONTROLLER_H
#define STACKCONTROLLER_H

#include <QObject>
#include "history.h"
#include "operationstack.h"
#include "samplingwidget.h"
#include "exportcontroller.h"


class StackController : public QObject
{
    Q_OBJECT

public:
    StackController(OperationStack* opStack, History* mipHistory, SamplingWidget* samplingWidget, ExportController* exportController, DataManager* dm);
    ~StackController();
    void select();


private:
    OperationStack* m_opStack;
    History* m_history;
    SamplingWidget* m_samplingWidget;
    ExportController* m_exportController;
    DataManager* m_dm;
    void deleteInvalidFuture();
    QMap<QString, QPair<int, QMap<QString, QVariant>>> m_algoSettings;
    int m_rowInEditMode = -1;


public slots:
    void slot_toggleKeyframe(uint idx, bool isNowKeyframe);
    void slot_deleteKeyframes();
    void slot_deleteAllKeyframes();
    void slot_rowClicked(int row);
    void slot_clearClicked();
    void slot_algorithmFinished(int index);
    void slot_keyframesChangedByPlugin(QString pluginName);
    void slot_exportFinished(QMap<QString, QVariant> settings);
    void slot_editClicked(int row);
    void slot_exitEditMode();
    void slot_startEdit();
};

#endif // STACKCONTROLLER_H
