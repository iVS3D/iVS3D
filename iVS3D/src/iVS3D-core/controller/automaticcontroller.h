#ifndef AUTOMATICCONTROLLER_H
#define AUTOMATICCONTROLLER_H

#include "model/automaticexecsettings.h"
#include "view/automaticwidget.h"
#include "view/outputwidget.h"
#include "view/samplingwidget.h"
#include "view/progressdialog.h"
#include "model/automaticexecutor.h"
#include <QObject>


/**
 * @class AutomaticController
 *
 * @ingroup Controller
 *
 * @brief The AutomaticController class is responsible for controlling the automatic execution
 *
 * @author Daniel Brommer
 *
 * @date 2022/07/08
 */


class AutomaticController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief AutomaticController Constructor of AutomaticController, this will create AutoExec and AutoExecSettings
     * @param outputWidget The current OutputWidget
     * @param autoWidget The current AutoWidget
     * @param samplingWidget The current SamplingWidget
     * @param dm The current DataManager
     */
    explicit AutomaticController(OutputWidget* outputWidget, AutomaticWidget* autoWidget, SamplingWidget* samplingWidget, DataManager* dm);
    /**
     * @brief autoExec Getter for AutoExec
     * @return AutoExec
     */
    AutomaticExecutor *autoExec();
    /**
     * @brief setExporController Sets the current exportController in the AutoExec classes
     * @param exCon The Current ExportController
     */
    void setExporController(ExportController *exCon);
    /**
     * @brief disableAutoWidget Disables the AutoWidget
     */
    void disableAutoWidget();

public slots:
    /**
     * @brief slot_setAlgorithm Sets the SamplingWidget to the given algorithm
     * @param index Index of the algorithm
     */
    void slot_setAlgorithm(int index);
    /**
     * @brief slot_saveConfiguration Connected to the AutomaticWidget, called to save the current settings to a JSON file
     */
    void slot_saveConfiguration();
    /**
     * @brief slot_loadConfiguration Connected to the AutomaticWidget, called to load a existing JSON file via a file dialog
     */
    void slot_loadConfiguration();
    /**
     * @brief slot_updatedSelectedPlugins Updates the plugins shown in the AutoWidget
     * @param usedPlugins QStringList with the current plugin list
     */
    void slot_updatedSelectedPlugins(QStringList usedPlugins);
    /**
     * @brief slot_createProgress Used to create a ProgressDialog (or TerminalInteraction) and connect signals from the AlgorithmExecutor
     * @param algoExec The AlgorithmExecutor which will report progress
     */
    void slot_createProgress(AlgorithmExecutor* algoExec);
    /**
     * @brief slot_deleteProgress Deletes the current ProgressDialog, if existing
     */
    void slot_deleteProgress();

signals:
    /**
     * @brief sig_saveConfiguration Signal emitted when 'Save Config' is clicked
     * @param path Path to the user selected file
     */
    void sig_saveConfiguration(QString path);
    /**
     * @brief sig_loadConfiguration Signal emitted when 'Load Config' is clicked
     * @param path Path to the user selected file
     */
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
