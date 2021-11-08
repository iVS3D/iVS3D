#include "itransformrequestdequeue.h"

ITransformRequestDequeue::ITransformRequestDequeue(ITransform *transform)
{
    m_transform = transform;
    connect(m_transform, &ITransform::sendToGui, this, &ITransformRequestDequeue::slot_sendToGui);
}

ITransformRequestDequeue::~ITransformRequestDequeue()
{
    disconnect(m_transform, &ITransform::sendToGui, this, &ITransformRequestDequeue::slot_sendToGui);
    delete m_transform;
}

QWidget *ITransformRequestDequeue::getSettingsWidget(QWidget *parent)
{
    return m_transform->getSettingsWidget(parent);
}

QString ITransformRequestDequeue::getName() const
{
    return m_transform->getName();
}

QStringList ITransformRequestDequeue::getOutputNames()
{
    return m_transform->getOutputNames();
}

ITransformRequestDequeue *ITransformRequestDequeue::copy()
{
    return new ITransformRequestDequeue(m_transform->copy());
}

ImageList ITransformRequestDequeue::transform(uint idx, const cv::Mat &img)
{
    return m_transform->transform(idx, img);
}

void ITransformRequestDequeue::enableCuda(bool enabled)
{
    m_transform->enableCuda(enabled);
}

void ITransformRequestDequeue::moveToThread(QThread *thread)
{
    m_transform->moveToThread(thread);
    QObject::moveToThread(thread);
}

void ITransformRequestDequeue::setSettings(QMap<QString, QVariant> settings)
{
    m_transform->setSettings(settings);
}

QMap<QString, QVariant> ITransformRequestDequeue::getSettings()
{
    return m_transform->getSettings();
}

void ITransformRequestDequeue::slot_transform(uint idx, const cv::Mat &img)
{
    m_imageToTransform = img;
    m_idxToTransform = idx;
    QTimer::singleShot(0, this, &ITransformRequestDequeue::slot_startTransform);
}

void ITransformRequestDequeue::slot_sendToGui(uint idx, const cv::Mat &img)
{
    emit sendToGui(idx, img);
}

void ITransformRequestDequeue::slot_startTransform()
{
    if(m_idxToTransform == UINT_MAX){
        return;
    }
    auto res = transform(m_idxToTransform, m_imageToTransform);
    emit sig_transformFinished(m_idxToTransform,res);
    m_idxToTransform = UINT_MAX;
}

void ITransformRequestDequeue::slot_enableCuda(bool enabled)
{
    enableCuda(enabled);
}
