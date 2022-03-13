#ifndef ITRANSFORM_H
#define ITRANSFORM_H

#include <QObject>
#include "opencv2/core.hpp"
#include "cvmat_qmetadata.h"
#include "iVS3D-pluginInterface_global.h"

/**
 * @interface ITransform
 *
 * @ingroup Plugin
 *
 * @brief The ITransform interface is used for algorithms to create additional image files from the source images. Plugins for this
 * interface are dynamically loaded from the plugins folder. Each ITransform needs to provide an QWidget for user interaction and a
 * display name. Using the transform method, the plugin can create additional images such as semantic maps for the given source image.
 *
 * Since calculation on images can be computationally expensive, the ITransform instance is moved to a worker thread. Previews for the
 * gui can be sent to the gui thread using the sendToGui signal.
 *
 * Keep in mind that the QWidget provided by getSettingsWidget will run
 * in the gui thread while the ITransform instance itself is moved to a worker thread. Intraction between the QWidget and ITransform
 * has to be threadsafe!
 *
 * @see SemanticSegmentation
 *
 * @date 2021/03/30
 *
 * @author Dominik Wüst
 *
 * This example shows how to create a ITransform plugin and a settings widget with a button:
 *
 * @code
 *  class Plugin : public ITransform {
 *      Q_OBJECT
 *      Q_PLUGIN_METADATA(IID "pse.iVS3D.ITransform")
 *      Q_INTERFACES(ITransform)
 *
 *  QWidget *settingsWidget = nullptr;
 *  cv::Mat image;
 *  uint imageIdx;
 *
 *  private slots:
 *      void onButtonPressed(){
 *          // this code runs in worker thread
 *          // compute some preview...
 *          cv::Mat preview = cv::convert(image, RGB2GREY);
 *          // ... and update gui
 *          emit sendToGui(imageIdx, preview);
 *      }
 *
 *  public:
 *      QWidget* getSettingsWidget(QWidget* parent){
 *          if(settingsWidget) return settingWidget;
 *          // create a widget with a button
 *          auto *button = new QPushButton(parent, "Press me");
 *          settingsWidget = new QWidget(parent);
 *          settingsWidget->layout()->addWidget(button);
 *          // connect button to Plugin using Qt DirectConnection
 *          connect(button, &QPushButton::pressed, this, &Plugin::onButtonPressed, Qt::DirectConnection);
 *      }
 *
 *      ImageList transform(uint idx, const cv::Mat &img){
 *          image = img;
 *          imageIdx = idx;
 *          // perform transformation and show result on gui
 *          auto res = doWork();
 *          emit sendToGui(imageIdx,res[0]);
 *          return res;
 *      }
 *
 *      // other methods here...
 *  };
 * @endcode
 *
 * The Button on the settingsWidget is connected to the plugin using the threadsafe DirectConnection. This allows
 * to calculate computationally expensive previews in the worker thread and update the gui after calculation has
 * finished.
 */
class IVS3DPLUGININTERFACE_EXPORT ITransform : public QObject
{
    Q_OBJECT

public:
    virtual ~ITransform() {}
    /**
     * @brief getSettingsWidget is provides an QWidget to display plugin specific settings to the user. Keep in mind that the widget
     * will run in gui thread while this ITransform instance is moved to a worker thread! Use DirectConnection for signals between the
     * widget and this instance or ensure thread safety for interaction.
     * @param parent The parent for the QWidget
     * @return The QWidget with the plugin settings
     */
    virtual QWidget* getSettingsWidget(QWidget* parent) = 0;

    /**
     * @brief getName returns the display name for the plugin.
     * @return The name to display.
     */
    virtual QString getName() const = 0;

    /**
     * @brief getOutputNames returns a list of folder names created on export.
     * @return The names for each folder
     */
    virtual QStringList getOutputNames() = 0;

    /**
     * @brief copy creates a new ITransform instance which is a deep copy.
     * @return The copy
     */
    virtual ITransform *copy() = 0;

    /**
     * @brief transform generates additional images from the given image. The signal sendToGui can be used to
     * update the preview for the user.
     * @param idx The index of the image to transform
     * @param img The image to transform
     * @return Pointers to the transformed images
     */
    virtual ImageList transform(uint idx, const cv::Mat &img) = 0;

    /**
     * @brief enableCuda enables use of the CUDA api to accelerate computations.
     * @param enabled The CUDA api is used if @a true
     */
    virtual void enableCuda(bool enabled) = 0;

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


signals:
    /**
     * @brief [signal] sendToGui is emitted if the ITransform instance has an image to display to the user.
     * @param idx The index of the image in transformation
     * @param img The image to display to the user
     */
    void sendToGui(uint idx, const cv::Mat &img);
};

Q_DECLARE_INTERFACE(ITransform, "pse.iVS3D.ITransform")

#endif // ITRANSFORM_H
