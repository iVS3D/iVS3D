#ifndef ITRANSFORMREQUESTDEQUEUE_H
#define ITRANSFORMREQUESTDEQUEUE_H

#include <itransform.h>

#include <QObject>
#include <QThread>
#include <QTimer>

/**
 * @class ITransformRequestDequeue
 *
 * @ingroup Plugin
 *
 * @brief The ITransformRequestDequeue class is a proxy for an ITransform. It is used to run ITransform::transform and
 * other local methods in a worker thread. Therefore the proxy overrides the QObject::moveToThread method and provides
 * slots and signals to interact with from the gui thread. Since requests from the gui thread usually come in much faster
 * than they can be handled by the worker thread, requests are collected and only the most recent one is passed to the
 * ITransform for computation.
 *
 * Note that while the ITransform instance is moved to the given
 * worker thread in moveToThread, the SettingsWidget created by this ITransform is NOT MOVED TO THE WORKER THREAD. This
 * reqeuires connections between the ITransform and the QWidget to be threadsafe!
 *
 * @see ITransform
 */
class ITransformRequestDequeue : public ITransform
{
    Q_OBJECT
public:
    /**
     * @brief ITransformRequestDequeue creates a proxy for the given ITransform.
     * @param transform The ITransform instance to hide behind the proxy
     */
    explicit ITransformRequestDequeue(ITransform *transform);
    ~ITransformRequestDequeue();

    /**
     * @brief getSettingsWidget is provides an QWidget to display plugin specific settings to the user. Keep in mind that the widget
     * will run in gui thread while this ITransform instance is moved to a worker thread!
     * @param parent The parent for the QWidget
     * @return The QWidget with the plugin settings
     */
    QWidget *getSettingsWidget(QWidget* parent) override;

    /**
     * @brief getName returns the display name for the plugin.
     * @return The name to display.
     */
    QString getName() const override;

    /**
     * @brief getOutputNames returns a list of folder names created on export.
     * @return The names for each folder
     */
    QStringList getOutputNames() override;

    /**
     * @brief copy creates a new ITransformRequestDequeue instance which is a deep copy.
     * @return The copy
     */
    ITransformRequestDequeue *copy() override;

    /**
     * @brief transform generates additional images from the given image. The signal sendToGui can be used to
     * update the preview for the user.
     * @param idx The index of the image to transform
     * @param img The image to transform
     * @return Pointers to the transformed images
     */
    ImageList transform(uint idx, const cv::Mat &img) override;

    /**
     * @brief enableCuda enables use of the CUDA api to accelerate computations.
     * @param enabled The CUDA api is used if @a true
     */
    void enableCuda(bool enabled) override;

    /**
     * @brief moveToThread moves this ITransformRequestDequeue with its ITransform instance to the given thread.
     * @param thread The worker thread to move to
     */
    void moveToThread(QThread *thread);

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    void setSettings(QMap<QString, QVariant> settings) override;

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    QMap<QString, QVariant> getSettings() override;

signals:
    /**
     * @brief [signal] sig_transformFinished is emitted after ITransform::transform ran.
     * @param idx The index of the transformed image
     * @param images The resulting images of ITransform::transform
     */
    void sig_transformFinished(uint idx, ImageList images);

public slots:
    /**
     * @brief [slot] slot_transform adds the given image to the waiting queue or starts transformation if queue is empty.
     * @param idx The index of the image to transform
     * @param img The image to transform
     */
    void slot_transform(uint idx, const cv::Mat &img);

    /**
     * @brief [slot] slot_startTransform starts transformation with the most recent image in the waiting queue.
     */
    void slot_startTransform();

    /**
     * @brief [slot] slot_enableCuda enables or disables the CUDA api for the ITransform
     * @param enabled Enables CUDA if @a true, disables CUDA otherwise.
     */
    void slot_enableCuda(bool enabled);

private slots:
    void slot_sendToGui(uint idx, const cv::Mat &img);

private:
    ITransform *m_transform;
    cv::Mat m_imageToTransform;
    uint m_idxToTransform;
};

#endif // ITRANSFORMREQUESTDEQUEUE_H
