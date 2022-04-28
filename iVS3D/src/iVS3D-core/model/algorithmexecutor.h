#ifndef ALGORITHMEXECUTOR_H
#define ALGORITHMEXECUTOR_H

#include "progressable.h"
#include "DataManager.h"
#include "algorithmthread.h"
#include "samplethread.h"
#include "settingsthread.h"
#include "algorithmthread.h"
#include "applicationsettings.h"
#include "stringcontainer.h"

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

struct ALGO_DATA {
    ModelInputPictures *mip;
    std::vector<uint> images;
    bool useCuda;
};

/**
 * @class AlgorithmExecutor
 *
 * @ingroup Model
 *
 * @brief The AlgorithmExecutor class is used to start the execution of a plugin(algorithm) and to provide signal&slots for the Thread executing the plugin
 *
 * @author Dominic Zahn
 *
 * @date 2021/02/18
 */
class AlgorithmExecutor : public Progressable
{
    Q_OBJECT
public:
    /**
     * @brief AlgorithmExecutor sets DataManager member variable
     * @param dataManager given DataManager to use model-data
     */
    explicit AlgorithmExecutor(DataManager *dataManager);
    /**
     * @brief startAlgorithm prepares data for plugin and starts its execution on new Thread
	 * @param useBounds true = respect boundaries and limit image-list accordingly
     * @return returns error code, 0 = no errors
     */
    int startSampling(int pluginIdx);
    /**
     * @brief startGenerateSettings prepares data for generateSettings and starts its execution on new Thread
     * @param pluginIdx selected plugin which gets executed
     * @return returns error code, 0 = no errors
     */
    int startGenerateSettings(int pluginIdx);

signals:
    /**
     * @brief sig_algorithmStarted signals start of plugin (unused
     */
    void sig_algorithmStarted();
    /**
     * @brief sig_algorithmAborted signals completed abortion of plugin
     */
    void sig_algorithmAborted();
    /**
     * @brief sig_algorithmFinished gets emitted once the plugin finished and all results are saved to the model
     */
    void sig_pluginFinished();

public slots:
    /**
     * @brief slot_abort gets called by GUI to start aborting the plugin
     */
    void slot_abort();
    /**
     * @brief slot_pluginFinished gets invoked by Thread emitting "Finished(..)", meaning it is finished with its work, method saves results of plugin into the model
     */
    void slot_pluginFinished();

private:
    ALGO_DATA prepareAlgoStart(int pluginIdx);

    int m_pluginIndex = 0;
    volatile bool m_stopped = false;
    bool m_algorithmFinished = false;
    bool m_blurFinished = false;
    DataManager *m_dataManager = nullptr;
    SampleThread *m_sampleThread = nullptr;
    QThread *m_currentThread = nullptr;
    SettingsThread *m_settingsThread = nullptr;
    QString m_currentOperation = "";
    int m_operationProgress = 0;
};

#endif // ALGORITHMEXECUTOR_H
