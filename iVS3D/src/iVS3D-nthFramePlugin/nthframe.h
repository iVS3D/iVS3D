#ifndef NTHFRAME_H
#define NTHFRAME_H

/** @defgroup NthFramePlugin NthFramePlugin
 *
 * @ingroup Plugin
 *
 * @brief Plugin to select every Nth frame as keyframe.
 */

#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QSpinBox>
#include <QLayout>
#include <QLabel>
#include <QSpinBox>
#include <QSizePolicy>

#include "ialgorithm.h"
#include "reader.h"
#include "progressable.h"
#include "nthframe_global.h"
#include "signalobject.h"

#define DESCRIPTION_TEXT "Every Nth frame is selected as keyframe."
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define NAME_N "N"

// log file
#define LF_TOTAL "Total"


/**
 * @class NthFrame
 *
 * @ingroup NthFramePlugin
 *
 * @brief The NthFrame class implements the IAlgorithm plugin interface and provides functionality needed to select every Nth frame as
 * keyframe. The class also provides methods for changing N and visualizing the plugin in the core application using the name and settings.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/14
 */
class NTHFRAME_EXPORT NthFrame : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "pse.iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)    // declare this as implementation of IAlgorithm interface

public:
    /**
     * @brief NthFrame Contructor to create an instance with N initialized to 1.
     */
    NthFrame();
    ~NthFrame();

    /**
     * @brief showSettings Show a SettingsDialog to the user. This allows to change the value of N.
     * @param parent The parent widget for the SettingsDialog. This is needed to determin style and visibility.
     */
    QWidget* getSettingsWidget(QWidget *parent);

    /**
     * @brief sampleImages Create an keyframe list with indices to every nth sharp image. Sharp images are specified by index in sharpImages list.
     * The algorithm reports progress to the Progressable *receiver by calling Progressable::slot_makeProgress. Setting the *stopped bool to @a true
     * exits the algorithm.
     *
     * @param images A Reader instance to access images or the image/video path.
     * @param sharpImages A list containing indices of all images classified sharp.
     * @param receiver A Progressable instance. The slot Progressable::slot_makeProgress is called to report progress as an int in range [0,100].
     * @param stopped Pointer to a bool inidcating if user wants to stop computation. Algorithm aborts if set to @a true.
     * @param useCuda @a true if cv::cuda can be used
     * @param logFile can be used to protocoll progress or problems
     * @return A list of indices. Each index represents a keyframe from the given images.
     */
    std::vector<uint> sampleImages(Reader *images, const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent *logFile);

    /**
     * @brief getName Returns a name for displaying this algorithm to the user.
     * @return the name as QString.
     */
    QString getName() const;

    /**
     * @brief getBuffer Returns a buffer for storeing previously calculated Infos (not used in n-th frame)
     * @return the buffer as a QVariant (empty)
     */
    QVariant getBuffer();

    /**
     * @brief getBufferName Returns the name of the buffer
     * @return the name of the Buffer as a QString
     */
    QString getBufferName();

    /**
     * @brief initialize Sets up the default value which is corresponding to video information
     * @param reader is used to get video or image information
     */
    void initialize(Reader* reader);

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    virtual void setSettings(QMap<QString, QVariant> settings);

    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> generateSettings(Progressable *receiver, QMap<QString, QVariant> buffer, bool useCuda, volatile bool* stopped);

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> getSettings();

    void setSignalObject(signalObject* sigObj);

public slots:
    /**
     * @brief slot_nChanged updates N.
     * @param n The new value for N
     */
    void slot_nChanged(int n);

    void slot_newMetaData();

private:
    void createSettingsWidget(QWidget *parent);

    unsigned int m_N;
    uint m_numFrames;
    QWidget *m_settingsWidget;
    int m_fps = 30;
    QSpinBox* m_spinBox = nullptr;
    signalObject* m_sigObj;
};

#endif // NTHFRAME_H
