#ifndef CAMERA_MOVEMENT
#define CAMERA_MOVEMENT

/** @defgroup CameraMovementPlugin CameraMovementPlugin
 *
 * @ingroup Plugin
 *
 * @brief Plugin to select frame as keyframe based on camera movement since last keyframe.
 */

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QMap>
#include <QSpacerItem>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QCheckBox>
#include <QLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QDebug>
#include <QElapsedTimer>
#include <iostream>
#include <iomanip>
#include <opencv2/core/mat.hpp>

#include "ialgorithm.h"
#include "reader.h"
#include "farnebackoptflowfactory.h"
#include <opencv2/video.hpp>

#define DELIMITER_COORDINATE "|"
#define DELIMITER_ENTITY ","
#define BUFFER_NAME "MovementValues"
#define PLUGIN_NAME "Camera Movement Detection"
#define MOVEMENTTHRESHOLD_DESCRIPTION "Defines the camera movement, which has to be between two keyframes in order for them to be picked. (How big it is depends on the resolution.)"
#define RESETDELTA_DESCRIPTION "Won't directly compare two pictures that are further apart than <resetDelta>."
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define INFO_PREFIX "INFO: "
#define INFO_DEFAULT "Sample over a dataset to get information about the previous computation."
#define INFO_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightGreen;"
#define RESET_DELTA "Reset delta"
#define CAMERA_MOVEMENT_THRESHOLD "Camera movement threshold"

// log file
#define LF_BUFFER "Buffer"
#define LF_OPT_FLOW_TOTAL "Flow calculation"

/**
 * @class CameraMovement
 *
 * @ingroup CameraMovementPlugin
 *
 * @brief The CameraMovement class implements the IAlgorithm interface and provides an algorithm for keyframe selection based on the camera movement between frames.
 *
 * @author Dominic Zahn
 *
 * @date 2021/02/15
 */
class CameraMovement : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "pse.iVS3D.IAlgorithm")   // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)                        // declare this as implementation of IAlgorithm interface

public:
    /**
     * @brief CameraMovement Constructor sets default values for member variables
     */
    CameraMovement();
    ~CameraMovement(){}

    /**
     * @brief getSettingsWidget creates a Widget, which can be used to change the algorithm parameters and returns it
     * @param parent is the parent of the newly created SettingsWidget
     * @return a settingWidget, which can be used to change the algorithm parameters
     */
    QWidget *getSettingsWidget(QWidget *parent) override;

    /**
     * @brief sampleImages selects keyframes based on the rotion between them
     * @param reader gives the method access to the video/image sequence, which should be used
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the already made progress
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param buffer is a QVariant, which holds previous computions that could be usefull for the next selection
     * @param useCuda defines if the compution should run on graphics card
     * @param logFile poiter to the log file
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(Reader *reader, const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent *logFile) override;

    /**
     * @brief getName Returns a name for displaying this algorithm to the user.
     * @return the name as QString.
     */
    QString getName() const override;

    /**
     * @brief getBuffer Returns a buffer for storeing previously calculated Infos
     * @return the buffer as a QVariant (empty)
     */
    QVariant getBuffer() override;

    /**
     * @brief getBufferName Returns the name of the buffer
     * @return the name of the Buffer as a QString
     */
    QString getBufferName() override;

    /**
     * @brief initialize Sets up the default value which is corresponding to video information
     * @param reader is used to get video or image information
     */
    void initialize(Reader* reader) override;

    /**
     * @brief setMovementThreshold Sets a new value for the movementThreshold parameter
     * @param movementThreshold defines the camera movement diffrence between two pictures for them to be picked as keyframe
     */
    void setMovementThreshold(double movementThreshold);

    /**
     * @brief setResetDelta Sets a new vaule for the resetDelta parameter
     * @param resetDelta defines a cut off point after which the old picture gets updated
     */
    void setResetDelta(int resetDelta);

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */

    virtual void setSettings(QMap<QString, QVariant> settings) override;

    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> generateSettings(Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool* stopped) override;

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> getSettings() override;

private slots:
    void movementThresholdChanged(QString sThreshold);

private:
    QColor m_defaultTextColor;
    double m_cameraThreshold = 2.0;
    int m_resetDelta = 5;
    Reader *m_reader = nullptr;
    QSpacerItem *m_movementSpacer;
    QDoubleValidator *m_doubleVal;
    QLineEdit *m_movementLineEdit;
    QSpinBox *m_resetSpinBox;
    QWidget *m_settingsWidget;
    QLabel *m_infoLabel;
    bool m_cuda = false;
    cv::SparseMat m_bufferMat;
    LogFileParent *m_logFile = nullptr;

    /**
     * @brief averageFlow calculates a value that represents a rotary movement.
     * To compute a representive value the function extracts his information from the given flow matrix.
     * @param flow matrix, which holds the raw flow information
     * @param stepSize is used to travers the matrix faster. However a bigger stepSize also means
     * @return most of the time a value between 0 and 10 that represents the rotaion between two images
     */
    double averageFlow(cv::Mat flow, uint stepSize);
    void calcOptFlowSingle(std::vector<uint> &keyframes, Reader *reader, std::vector<unsigned int> sharpImages, Progressable *receiver, volatile bool *stopped);
    /**
     * @brief recreateBufferMatrix initalizes the buffer matix whith the new values from nBuffer
     * @param nBuffer holds the new movement values which should be stored in the buffer matrix
     */
    void recreateBufferMatrix(QMap<QString, QVariant> nBuffer);
    /**
     * @brief recreateMovementFromString is used to extract the movement value from a string and write it in the buffer
     * @param string that holds the two compared images indices and the movement value (most likeyl from settings.json)
     */
    void stringToBufferMat(QString string);
    void createSettingsWidget(QWidget *parent);
    void updateInfoLabel(double averageMovement);
};

#endif // CAMERAMOVEMENT_H
