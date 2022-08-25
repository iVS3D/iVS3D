#ifndef IALGORITHM_H
#define IALGORITHM_H

#include <QObject>
#include "opencv2/core.hpp"
//#include "cvmat_qmetadata.h"
#include "iVS3D-pluginInterface_global.h"
#include <QDebug>

#include "reader.h"
#include "progressable.h"
#include "LogFileParent.h"
#include "signalobject.h"

/**
 * @interface IAlgorithm
 *
 * @ingroup Plugin
 *
 * @brief The IAlgorithm interface is used to include plugin implementations for different sampling algorithms. As such the plugins need to present a display name
 * and have the possibility to display a custom QWidget to the user. The main task for a IAlgorithm plugin to select keyframes based on the given images. This is
 * performed in sampleImages. Images are accessible by the Reader object. The currently selected images are apssed as indices in imageList. This allows the plugin
 * to sample on the current image selection as well as include intermediate images in the computation. The plugin can write data to an QVariant in order to buffer them.
 * Buffered data is managed by the core application and is passed to the plugin on the next sampleImages call.
 *
 * For implementation examples see NthFrame, Blur and Rotation.
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/14
 */


class IVS3DPLUGININTERFACE_EXPORT IAlgorithm : public QObject
{
    Q_OBJECT

public:
    virtual ~IAlgorithm() {}
    /**
     * @brief getSettingsWidget is provides an QWidget to display plugin specific settings to the user. The Widget is not deleted by the core application,
     * so storage management is duty of the plugin.
     * @param parent The parent for the QWidget
     * @return The QWidget with the plugin settings
     */
    virtual QWidget* getSettingsWidget(QWidget* parent) = 0;

    /**
     * @brief getName returns the display name for the plugin. This name is pesented to the user.
     * @return The name to display
     */
    virtual QString getName() const = 0;

    /**
     * @brief sampleImages selects the keyframes from the given images. The computation is based on the images provided by the given Reader. The imageList
     * provides indices for the currently selected keyframes. This way the current selection can be taken in account in the calculation. Since the algorithm
     * can be very timeconsuming, it is recommended to report progress to the given Progressable by invoking Progressable::slot_makeProgress. It should be
     * possible to abort calculation by setting the value of stopped to @a true. Therefore it is necessary to repeatedly check its state. To speedup calculation,
     * data can be buffered to accelerate the next call of sampleImages. On some systems the gpu can be utalized for acceleration as well, but only if useCuda is
     * @a true. See following example implementation for sampleImages:
     *
     * @code
     *  // store result in here
     *  std::vector<uint> keyframes;
     *
     *  for(int i = 0; i < imageList.size(); ++i){
     *      // fetch the image
     *      uint imageidx = imageList[i];
     *      cv::Mat *image = reader.getPic(imageidx);
     *
     *      // calculate if image is keyframe
     *      // doWork could read from or write to the buffer
     *      // if useCude == true cv::cuda implementations can be used
     *      int result = doWork(image);
     *      if(result == IS_KEYFRAME){
     *          keyframes.push_back(imageindex);
     *      }
     *
     *      // send new progress update
     *      int progress = i * 100 / imageList.size();
     *      QMetaObject::invokeMethod(
     *           receiver,
     *           "slot_makeProgress",
     *           Qt::DirectConnection,
     *           Q_ARG(int, progress),
     *           Q_ARG(QString, "computed image " + QString::number(i) + " of " + QString::number(imageList.size())));
     *
     *      // if user wants to abort then return
     *      if(*stopped) {
     *          return std::vector<uint>();
     *      }
     *  }
     *
     *  return keyframes;
     * @endcode
     *
     * @param imageList Index list of images to compute, but indices inbetween can be used for computation (see Blur)
     * @param receiver The Progressable to invoke to report progress
     * @param stopped Flag @a true if the computation should abort, @a fals if it should continue
     * @param useCuda @a true if cv::cuda can be used
     * @param logFile can be used to protocoll progress or problems
     * @return The indices of the seletced keyframes
     *
     * @see Rotation, Blur
     */
    virtual std::vector<uint> sampleImages(const std::vector<uint> &imageList, Progressable* receiver, volatile bool* stopped, bool useCuda, LogFileParent *logFile) = 0;

    /**
     * @brief initialize the the IAlgorithm and the settings widget with plusible values from the Reader.
     * @param reader The reader with the images
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param sigObj provides signals from the core application
     */
    virtual void initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj) = 0;

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    virtual void setSettings(QMap<QString, QVariant> settings) = 0;

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> getSettings() = 0;

    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @return QMap with the settings
     */
    virtual QMap<QString, QVariant> generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped) {return {}; };

signals:
    /**
     * @brief updateKeyframes updates the keyframe list in the core object.
     * @param keyframes the new keyframes
     */
    void updateKeyframes(std::vector<uint> keyframes);

    /**
     * @brief updateBuffer stores the given buffer for future uses of the buffered data.
     * @param buffer the data to store
     */
    void updateBuffer(QMap<QString, QVariant> buffer);
};

Q_DECLARE_INTERFACE(IAlgorithm, "iVS3D.IAlgorithm")

#endif // IALGORITHM_H
