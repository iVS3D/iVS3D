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
#include <QCheckBox>
#include <QTranslator>
#include <QCoreApplication>

#include "ialgorithm.h"
#include "reader.h"
#include "progressable.h"
#include "signalobject.h"

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
class NthFrame : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier
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
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the already made progress
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param useCuda defines if the compution should run on graphics card
     * @param logFile can be used to protocoll progress or problems
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent* logFile) override;
    /**
     * @brief getName Returns the plugin Name
     * @return "Blur"
     */
    QString getName() const override;
    /**
     * @brief initialize Blur doesn't use this currently
     * @param reader the images
     * @param buffer is a QVariant, which holds previous computions that could be usefull for the next selection
     * @param sigObj provides signals from the core
     */
    void initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj) override;
    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    void setSettings(QMap<QString, QVariant> settings) override;
    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    QMap<QString, QVariant> generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped) override;
    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    QMap<QString, QVariant> getSettings() override;


private slots:
    void slot_nChanged(int n);
    void slot_checkboxToggled(bool checked);

private:
    void createSettingsWidget(QWidget *parent);
    Reader *m_reader;
    unsigned int m_N;
    bool m_keepLonely;
    uint m_numFrames;
    QWidget *m_settingsWidget;
    int m_fps = 30;
    QSpinBox* m_spinBox = nullptr;
    QCheckBox* m_checkBox = nullptr;
};

#endif // NTHFRAME_H
