#ifndef CONTROLLER_H
#define CONTROLLER_H

/** @defgroup StationaryCameraPlugin StationaryCameraPlugin
 *
 * @ingroup Plugin
 *
 * @brief Plugin to remove keyframes if no camera movement was detected.
 */

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QSpacerItem>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QCheckBox>
#include <QLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <opencv2/core/mat.hpp>
#include <iostream>
#include <iomanip>
#include <QtConcurrent/QtConcurrentMap>
#include <QThreadPool>
#include <QTranslator>
#include <QCoreApplication>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>

#include "ialgorithm.h"
#include "reader.h"
#include "factory.h"
#include "imagegatherer.h"
#include "flowcalculator.h"
#include <opencv2/video.hpp>

#define PLUGIN_NAME QObject::tr("Smooth camera movement")
// widget
#define SELECTOR_LABEL_TEXT QObject::tr("Movement Threshold")
#define SELECTOR_DESCRIPTION QObject::tr("A resolution dependend threshold, that specifies when there was enough movement to set a new keyframe.")
#define DOWNSAMPLE_LABEL_TEXT QObject::tr("Sampling resolution")
#define DOWNSAMPLE_CHECKBOX_TEXT QObject::tr("Activate down sampling")
#define DESCRIPTION_DOWNSAMPLE QObject::tr("If enabled a resolution of 720p will be used for the algorithm to speed up computation. This however will hurt the accuracy of the result slightly. It however won't change the export resolution. This parameter will be disabled if the input resolution is lower or equal than 720p.")
#define RESET_BT_TEXT QObject::tr("Reset Buffer")
#define RESET_TEXT_PRE QObject::tr("There are ")
#define RESET_TEXT_SUF QObject::tr(" flow values currently buffered.")
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define INFO_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightGreen;"
// buffer
#define BUFFER_NAME "SmoothCameraMovementBuffer"
#define DELIMITER_COORDINATE "|"
#define DELIMITER_ENTITY ","
// settings
#define SETTINGS_SAMPLE_RESOLUTION "Sample resolution"
#define SETTINGS_SELECTOR_THRESHOLD "Selector threshold"
// log file
#define LF_OPT_FLOW_TOTAL "Flow calculation"
#define LF_SELECT_FRAMES "Selection of keyframes"
#define LF_CE_TYPE_ADDITIONAL_INFO "Additional Computation Information"
#define LF_CE_VALUE_USED_BUFFERED "Used buffered values"
#define LF_CE_TYPE_DEBUG "Debug Information"
#define LF_CE_NAME_FLOWVALUE "Flow value"
#define LF_CE_NAME_SAMPLERES "Sampling Resolution"
#define LF_TIMER_BUFFER "Update Buffer"
#define LF_TIMER_CORE "Core Computation"
#define LF_TIMER_SELECTION "Keyframe selection"

/**
 * @class StationaryCamera
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The StationaryCamera class implements the IAlgorithm interface and provides an algorithm to remove keyframes at positions where no movement of the camera was detected
 *
 * @author Dominic Zahn
 *
 * @date 2022/3/13
 */
class SmoothController : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm")   // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)                        // declare this as implementation of IAlgorithm interface

public:
    /**
     * @brief StationaryCamera Constructor sets default values for member variables
     */
    SmoothController();
    ~SmoothController() {}

    /**
     * @brief getSettingsWidget creates a Widget, which can be used to change the algorithm parameters and returns it
     * @param parent is the parent of the newly created SettingsWidget
     * @return a settingWidget, which can be used to change the algorithm parameters
     */
    QWidget *getSettingsWidget(QWidget *parent) override;

    /**
     * @brief sampleImages selects keyframe if the camera is currently not stationary
     * @param reader gives the method access to the video/image sequence, which should be used
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the already made progress
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param logFile poiter to the log file
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(const std::vector<uint> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent *logFile) override;

    /**
     * @brief getName Returns a name for displaying this algorithm to the user.
     * @return the name as QString.
     */
    QString getName() const override;

    /**
     * @brief initialize Sets up the default value which is corresponding to video information
     * @param reader is used to get video or image information
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param sigObj provides signals from the core application
     */
    void initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *sigObj) override;

    /**
     * @brief setter for plugin's settings
     * settings structure:
     *  <QMap> settings (all settings)
     *      <QVariant> samplingResolution (enable/disable diffrent resolution for sampling)
     *      <QVariant> selector_threshold
     *  </QMap>
     * @param QMap with the settings
     */
    void setSettings(QMap<QString, QVariant> settings) override;

    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    QMap<QString, QVariant> generateSettings(Progressable *receiver, bool useCuda, volatile bool *stopped) override;

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    QMap<QString, QVariant> getSettings() override;

private:
    // member variables
    double m_selectorThreshold = 2.0;
    double m_downSampleFactor = 1.0;
    Reader *m_reader = nullptr;
    QPoint m_inputResolution = QPoint(0, 0);
    cv::SparseMat m_bufferMat;
    signalObject *m_sigObj = nullptr;
    //      widget elements
    QWidget *m_settingsWidget = nullptr;
    QDoubleSpinBox *m_selectorThresholdSpinBox = nullptr;
    static constexpr double m_downSampleFactorArray[] = { 1.0, 1.5, 2.0, 2.5, 3.0, 4.0 };
    QCheckBox *m_downSampleCheck = nullptr;
    QPushButton *m_resetBufferBt = nullptr;
    QLabel *m_resetBufferLabel = nullptr;
    // timing variables
    long m_durationFarnebackMs = 0;
    long m_durationComputationFlowMs = 0;

    // functions
    bool downInputResToCheck(QPointF inputRes);
    bool downFactorToCheck(double downFactor);
    double downCheckToFactor(bool boxChecked, QPointF inputRes);
    void reportProgress(QString op, int progress, Progressable *receiver);
    void displayMessage(QString txt, Progressable *receiver);
    void createSettingsWidget(QWidget *parent);
    void resetBuffer();
    /**
     * @brief sendBuffer Sends all buffered values for storeing previously calculated Infos
     * @return the buffer as a QVariant (empty)
     */
    QMap<QString, QVariant> sendBuffer();
    /**
     * @brief updateBufferInfo updates the amount of buffered values in the status tip.
     * @param bufferedValueCount is the new amout of buffered flow values
     */
    QString updateBufferInfo(long bufferedValueCount);
    /**
     * @brief recreateBufferMatrix initalizes the buffer matix whith the new values from nBuffer
     * @param buffer holds the new movement values which should be stored in the buffer matrix
     */
    void recreateBufferMatrix(QMap<QString, QVariant> buffer);
    /**
     * @brief recreateMovementFromString is used to extract the movement value from a string and write it in the buffer
     * @param string that holds the two compared images indices and the movement value (most likeyl from settings.json)
     */
    void stringToBufferMat(QString string);
    QVariant bufferMatToVariant(cv::SparseMat bufferMat);
private slots:
    void sampleCheckChanged(bool isChecked);
signals:
    void changeUIParameter(QVariant nValue, QString paramName, QString selectorName);
};

#endif // CONTROLLER_H
