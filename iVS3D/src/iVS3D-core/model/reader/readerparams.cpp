#include "readerparams.h"
#include "stringcontainer.h"
#include <QDebug>
#include <QJsonObject>

void ReaderParams::initialize(const Resolution &originalResolution)
{
    m_originalResolution = originalResolution;
    m_workingResolution = originalResolution;
    m_roi = ROI();
    m_useRoi = false;
}

bool ReaderParams::setWorkingResolution(const Resolution &resolution)
{
    // make sure the working resolution is less then the original and is valid (i.e. not (0x0))
    if (resolution.getWidth() > m_originalResolution.getWidth() || resolution.getHeight() > m_originalResolution.getHeight()) return false;
    if (!resolution.isValid()) return false;

    m_workingResolution = resolution;
    return true;
}

bool ReaderParams::setRoi(const ROI &roi)
{
    m_roi = roi;
    return true;
}

QVariant ReaderParams::toText()
{
    QJsonObject json;
    json["original_width"] = static_cast<int>(m_originalResolution.getWidth());
    json["original_height"] = static_cast<int>(m_originalResolution.getHeight());
    json["working_width"] = static_cast<int>(m_workingResolution.getWidth());
    json["working_height"] = static_cast<int>(m_workingResolution.getHeight());
    json["roi_x"] = m_roi.toQRectF().x();
    json["roi_y"] = m_roi.toQRectF().y();
    json["roi_width"] = m_roi.toQRectF().width();
    json["roi_height"] = m_roi.toQRectF().height();
    json["use_roi"] = m_useRoi;
    return QVariant(json);
}

void ReaderParams::fromText(QVariant data)
{
    QJsonObject json = data.toJsonObject();
    m_originalResolution = Resolution(json["original_width"].toInt(), json["original_height"].toInt());
    m_workingResolution = Resolution(json["working_width"].toInt(), json["working_height"].toInt());
    QRectF roi(json["roi_x"].toDouble(), json["roi_y"].toDouble(), json["roi_width"].toDouble(), json["roi_height"].toDouble());
    m_roi = ROI(roi);
    m_useRoi = json["use_roi"].toBool();
}
