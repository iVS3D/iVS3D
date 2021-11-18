#ifndef BLUR_H
#define BLUR_H

/** @defgroup BlurPlugin BlurPlugin
 *
 * @ingroup Plugin
 *
 * @brief Calculates keyframes based on blurrines
 */

#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QDoubleSpinBox>
#include <QLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QComboBox>
#include <iostream>

#include "IAlgorithm.h"
#include "reader.h"
#include <opencv2/video.hpp>
#include "blurlaplacian.h"
#include "blursobel.h"
#include "BlurAlgorithm.h"
#include "logfile.h"


#define PLUGIN_NAME "Blur"
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define WINDOW_SIZE "Window size"
#define LOCAL_DEVIATION "Local deviation"
#define USED_BLUR "Blur"


/**
 * @class Blur
 *
 * @ingroup BlurPlugin
 *
 * @brief The Blur class is responsible for selecting they keyframes based on the blu values calculated by a BlurAlgorithm.
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/19
 */

class Blur : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "pse.iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)    // declare this as implementation of IAlgorithm interface

public:
    /**
     * @brief Blur Constructor which creates an instance of every BlurAlgorithm and stets standard values (WindowSize = 10, LocalDeviation = 95)
     */
    Blur();
    /**
     * @brief getSettingsWidget Returns the settings widget where BlurAlgorithm, WindowSizw and LocalDeviation can be set
     * @param parent Parent of the created QWidget
     * @return The Settings Widget
     */
    QWidget *getSettingsWidget(QWidget *parent) override;
    /**
     * @brief sampleImages selects keyframes based on their bluriness. It differs between a calulation on all images and a caculation only on keyframes.
     *
     * If allImages are used, sampleImages calulates the blur value of every image with the selected BlurAlgorithm. Then it creates a window for every image x.
     * This window is defined as [x - windowSize, .., x, .. , x + windowSize] (This window will be cropped to 0 on the left and picCOunt on the rigth side).
     * For every window the average Blur values y of it's images is calculated. If (Blur value from x) / y >= LocalDeviation / 100 Image x is considered a sharpImage.
     *
     * If only keyframes are used, sampleImages creates the same windows, but only for keyframes and it will selected the Image in a window with the largest blur value.
     *
     * @param reader gives the method access to the video/image sequence, which should be used
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the already made progress
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param buffer is a QVariant, which holds previous computions that could be usefull for the next selection
     * @param useCuda defines if the compution should run on graphics card
     * @param logFile can be used to protocoll progress or problems
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(Reader *reader, const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, QMap<QString, QVariant> buffer, bool useCuda, LogFileParent* logFile) override;
    /**
     * @brief getName Returns the plugin Name
     * @return "Blur"
     */
    QString getName() const override;
    /**
     * @brief Returns the calculated buffer which is a QVariant containing the calulated Blur Values
     * @return QVariant containing the calulated Blur Values
     */
    QVariant getBuffer() override;
    /**
     * @brief getBufferName Returns the buffer name
     * @return Name of the used BlurAlgorithm
     */
    QString getBufferName() override;
    /**
     * @brief initialize Blur doesn't use this currently
     * @param reader
     */
    void initialize(Reader* reader) override;    
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


public slots:
    /**
     * @brief [slot] slot_blurChanged selects the algorithm identified by name.
     * @param name The algorithm name
     */
    void slot_blurChanged(const QString & name);
    /**
     * @brief [slot] slot_wsChanged updates the windowsize.
     * @param ws The new windowsize
     */
    void slot_wsChanged(int ws);
    /**
     * @brief [slot] slot_ldChanged updates local deviation.
     * @param ld The new local deviation
     */
    void slot_ldChanged(int ld);


private:
    QWidget *m_settingsWidget;
    QComboBox* m_comboBoxBlur  = nullptr;
    QSpinBox* m_spinBoxWS  = nullptr;
    QSpinBox* m_spinBoxLD  = nullptr;
    void createSettingsWidget(QWidget *parent);
    BlurAlgorithm* m_usedBlur;
    int m_windowSize;
    double m_localDeviation;
    std::vector<BlurAlgorithm*> m_blurAlgorithms = {};
    std::vector<double> m_blurValues;
    LogFileParent *m_logFile = nullptr;
    std::vector<double> splitDoubleString(QString string);
    std::vector<uint> sampleAllImages(Reader *reader, Progressable *receiver, volatile bool *stopped, int start, int end);
    std::vector<uint> sampleKeyframes(Reader *reader, Progressable *receiver, volatile bool *stopped, std::vector<uint> sharpImages);
};

#endif // BLUR_H
