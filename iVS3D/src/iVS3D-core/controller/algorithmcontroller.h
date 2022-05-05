#ifndef ALGORITHMCONTROLLER_H
#define ALGORITHMCONTROLLER_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QTimer>
#include <QElapsedTimer>

#include "model/DataManager.h"
#include "model/algorithmexecutor.h"

#include "view/progressdialog.h"
#include "view/samplingwidget.h"

#include "plugin/algorithmmanager.h"
#include "plugin/transformmanager.h"

#define NO_IMAGE -1
#define NO_TRANSFORM -1

/**
 * @class AlgorithmController
 *
 * @ingroup Controller
 *
 * @brief The AlgorithmController class is responsible for controlling the user input regarding algorithm selection and execution.
 *
 * @author Patrick Binder
 *
 * @date 2021/01/28
 */

class AlgorithmController: public QObject
{
    Q_OBJECT
public:
    /**
     * @brief AlgorithmController Constructor
     *
     * @param dataManager The programm wide DataManger
     * @param samplingWidget A pointer to a SamplingWidget, on which all user input regarding algorithm selection and execution is done.
     */
    explicit AlgorithmController(DataManager* dataManager, SamplingWidget* samplingWidget);
    ~AlgorithmController();

public slots:
    /**
     * @brief Slot which is called, when the selected Algorithm is changed to a IAlgorithm. The slot will show the settings widget from the now selected Algorithm
     *
     * @param idx Index of the now selected Algorithm
     */
    void slot_selectAlgorithm(int idx);
    /**
     * @brief Slot which is called, when the selected Algorithm is changed to a Transform algortihm. The slot will show the settings widget from the now selected Algorithm
     *
     * @param idx Index of the now selected Transform algortihm
     */
    void slot_selectTransform(int idx);
    /**
     * @brief Slot which is called, when 'Sample images' is clicked. The slot will start the algorithm by calling the AlgorithmExecutor
     */
    void slot_startAlgorithm();
    /**
     * @brief Slot which is called, when 'Generate settings' is clicked. The slot will start the algorithm by calling the AlgorithmExecutor
     */
    void slot_startGenerateSettings();
    /**
     * @brief Slot which is called, when the algorithm finished. This will close the ProgressDialog
     *
     */
    void slot_algorithmFinished(int idx);
    /**
     * @brief Slot which is called, when the 'Abort' on the ProgressDialog is clicked. This will close the ProgressDialog
     *
     */
    void slot_algorithmAborted();

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

public slots:
    void slot_updateKeyframes(QString pluginName, std::vector<uint> keyframes);
    void slot_updateBuffer(QString pluginName, QMap<QString, QVariant> buffer);

private slots:
    void slot_previewStateChanged(bool enabled);


private:
    enum PluginType{
        Transform,
        Algorithm
    };
    DataManager* m_dataManager;
    SamplingWidget *m_samplingWidget;
    AlgorithmExecutor* m_algExec;
    ProgressDialog* m_algorithmProgressDialog;

    // used to compute transformation in separate thread using QtConcurrent
    QFutureWatcher<std::vector<cv::Mat*>> m_fWatcher;   // QFutureWatcher waiting for transformation to finish
    // <image, image_id, transform_idx>
    std::tuple<cv::Mat*,int, int> m_future;             // image currently in transformation
    std::tuple<cv::Mat*,int, int> m_fQueue;             // Queue for next transformation to perform
    void startNextTransform();                          // starts transformation of m_fQueue

    // selected plugin
    int m_pluginIdx;
    PluginType m_pluginType;

    // plugin runtime
    QElapsedTimer m_timer;
};
#endif // ALGORITHMCONTROLLER_H
