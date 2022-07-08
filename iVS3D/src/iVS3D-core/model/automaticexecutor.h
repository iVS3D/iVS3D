#ifndef AUTOMATICEXECUTOR_H
#define AUTOMATICEXECUTOR_H

#include "DataManager.h"
#include "algorithmexecutor.h"
#include "algorithmmanager.h"
#include "automaticexecsettings.h"
#include "nouiexport.h"
#include "stringcontainer.h"


/**
 * @class AutomaticExecutor
 *
 * @ingroup Model
 *
 * @brief The AutomaticExecutor class is used to run multiple algorithms sequential on the same input.
 *
 * @author Daniel Brommer
 *
 * @date 2021/06/10
 */

class AutomaticExecutor : public Progressable
{
    Q_OBJECT

public:
    /**
     * @brief AutomaticExecutor Connects signals and sets the AutomaticWidget to enabled (if existing)
     * @param dm Pointer to the DataManager
     * @param autoWidget Pointer to the AutomaticWidget
     * @param autoSettings Pointer to the AutomaticExecSettings
     * @param exportController Pointer to the ExportController
     */
    AutomaticExecutor(DataManager* dm, AutomaticExecSettings* autoSettings);
    ~AutomaticExecutor();
    /**
     * @brief startMultipleAlgo Executes the indexed algorithm
     * @param algoList List of all selected algorithms
     * @param step Index of the current algorithm
     */
    void startMultipleAlgo(QList<QPair<QString, QMap<QString, QVariant>>> algoList, int step);
    /**
     * @brief isFinished Shows if all algorithms are done
     * @return @a true if all algorithms are done @a false otherwise
     */
    bool isFinished();

    /**
     * @brief setExportController ExportController doesn't exists when this class is created, so it has to be set later
     * @param exportController Pointer to the ExportController
     */
    void setExportController(ExportController* exportController);

public slots:
    /**
     * @brief slot_startAutomaticExec Connected to the AutomaticWidget, used to start the automatic execution
     */
    void slot_startAutomaticExec();
    /**
     * @brief slot_algoFinished Connected to the current algorithm executor, called when the current algorithm finished
     */
    void slot_samplingFinished();
    /**
     * @brief slot_algoAbort Connected to the current algorithm executor, called when a algorithm is aborted. This will stop the auto execution completly.
     */
    void slot_algoAbort();
    /**
     * @brief slot_settingsFinished Connected to the current algorithm executor, called when generate settings finished
     */
    void slot_settingsFinished();


signals:
    /**
     * @brief [signal] sig_hasStatusMessage() is emitted when the status message is updated.
     *
     * @param message QString with the new status
     */
    void sig_hasStatusMessage(QString message);

    /**
     * @brief [signal] sig_stopPlay() is emitted when an algorithm is started to stop the VideoPlayer.
     *
     */
    void sig_stopPlay();


    void sig_createProgress(AlgorithmExecutor* algoExec);

    void sig_deleteProgress();

private:
    int stepToPluginIndex(int step);
    void executeSampling();
    void executeExport(QMap<QString, QVariant> settings);
    DataManager *m_dm;
    AlgorithmExecutor* m_algoExec;
    int m_step = 0;
    int m_stepCount;
    QList<QPair<QString, QMap<QString, QVariant>>> m_pluginOrder;
    AutomaticExecSettings *m_autoSettings;
    ExportController *m_exportController;
    bool m_isFinished = false;
    noUIExport *m_exportRunner = nullptr;

};

#endif // AUTOMATICEXECUTOR_H
