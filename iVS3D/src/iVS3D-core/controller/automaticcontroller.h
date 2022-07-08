#ifndef AUTOMATICCONTROLLER_H
#define AUTOMATICCONTROLLER_H

#include "model/automaticexecsettings.h"
#include "view/automaticwidget.h"
#include "view/outputwidget.h"
#include "view/samplingwidget.h"
#include "view/progressdialog.h"
#include "model/automaticexecutor.h"
#include <QObject>


class AutomaticController : public QObject
{
    Q_OBJECT
public:
    explicit AutomaticController(OutputWidget* outputWidget, AutomaticWidget* autoWidget, SamplingWidget* samplingWidget, DataManager* dm);

    AutomaticExecSettings *autoExecSettings();

    void setExporController(ExportController *exCon);
    AutomaticExecutor *autoExec();

public slots:
    void slot_setAlgorithm(int index);
    /**
     * @brief slot_saveConfiguration Connected to the AutomaticWidget, called to save the current settings to a JSON file
     */
    void slot_saveConfiguration();
    /**
     * @brief slot_loadConfiguration Connected to the AutomaticWidget, called to load a existing JSON file via a file dialog
     */
    void slot_loadConfiguration();

    void slot_updatedSelectedPlugins(QStringList usedPlugins);

    void slot_createProgress(AlgorithmExecutor* algoExec);

    void slot_deleteProgress();

signals:

    void sig_saveConfiguration(QString path);

    void sig_loadConfiguration(QString path);


private:
    OutputWidget* m_outputWidget;
    AutomaticWidget* m_autoWidget;
    SamplingWidget* m_samplingWidget;
    AutomaticExecSettings* m_autoExecSettings;
    AutomaticExecutor* m_autoExec;
    ProgressDialog* m_algorithmProgressDialog;
    TerminalInteraction* m_terminal;
};


#endif // AUTOMATICCONTROLLER_H
