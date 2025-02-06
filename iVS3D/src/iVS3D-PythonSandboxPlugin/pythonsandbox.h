#ifndef PYTHONSANDBOX_H 
#define PYTHONSANDBOX_H 

/** @defgroup PythonSandboxPlugin PythonSandboxPlugin 
 * 
 * @ingroup Plugin
 *
 * @brief Insert your description.
 */

#define PYBIND11_NO_KEYWORDS
#include <pybind11/embed.h>

#include <QObject> 
#include <QWidget> 
#include <QString> 
#include <QMap> 
#include <QDoubleSpinBox>
#include <QLayout> 
#include <QLabel> 
#include <QSizePolicy> 
#include <QTranslator> 
#include <QCoreApplication> 
#include <QRandomGenerator>

#include "ialgorithm.h"
#include "reader.h"
#include "progressable.h"
#include "signalobject.h"

#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;" 

// log file 
#define LF_TOTAL "Total" 
#define NAME_SURVIVAL_PROBABILITY "survival_prob"


/** 
 * @class PythonSandbox 
 * 
 * @ingroup PythonSandboxPlugin 
 * 
 * @brief Short description. 
 * 
 * @author Author 
 */ 
class PythonSandbox : public IAlgorithm 
{ 
    Q_OBJECT 
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier 
    Q_INTERFACES(IAlgorithm)    // declare this as implementation of IAlgorithm interface 

public: 
    PythonSandbox(); 
    ~PythonSandbox(); 

    /** 
     * @brief returns a widget to change parameters of this plugin. 
     * @param parent The parent for the SettingsWidget. 
     */ 
    QWidget* getSettingsWidget(QWidget *parent) override;

    /** 
     * @brief sampleImages Create an keyframe list with indices of the keyframes. The algorithm reports progress 
     * to the Progressable *receiver by calling Progressable::slot_makeProgress. If the *stopped bool is set to @a true 
     * the algorithm should abort the calculations and exit. 
     * 
     * @param imageList is a preselection of frames 
     * @param receiver is a progressable, which displays the progress made so far
     * @param stopped Pointer to a bool indication if user wants to stop the computation 
     * @param useCuda defines if the compution should run on graphics card 
     * @param logFile can be used to protocoll progress or problems 
     * @return A list of indices, which represent the selected keyframes. 
     */ 
    std::vector<uint> sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent* logFile) override; 
    /** 
     * @brief getName Returns the plugin name 
     * @return "PythonSandbox" 
     */ 
    QString getName() const override; 
    /** 
     * @brief initialize is called after new images have been loaded. the plugin parameters can be selected based on the images. 
     * @param reader for the images 
     * @param buffer contains persistently stored plugin data from the project file. This includes data stored using getSettings() and generateSettings()
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
    void slot_survivalProbChanged(double probability);

private: 
    void createSettingsWidget(QWidget *parent); 
    Reader *m_reader = nullptr;
    signalObject *m_signalObject = nullptr;
    double m_survivalProbability = 1.0;
    QWidget *m_settingsWidget = nullptr;
    QDoubleSpinBox* m_spinBox = nullptr;
}; 

#endif // PYTHONSANDBOX_H 
