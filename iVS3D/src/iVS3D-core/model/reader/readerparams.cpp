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



ROI::ROI(const QRect &roi, const Resolution &resolution) : m_roi(0.0,0.0,1.0,1.0)
{
    Q_ASSERT(resolution.isValid());
    if (!resolution.isValid()) {
        return;
    }

    Q_ASSERT(roi.width() < int(resolution.getWidth()));
    Q_ASSERT(roi.height() < int(resolution.getHeight()));

    double width = resolution.getWidth();
    double height = resolution.getHeight();
    m_roi = QRectF(roi.left()/width, roi.top()/height, roi.width()/width, roi.height()/height);
}

ROI::ROI(const cv::Rect &roi, const Resolution &resolution)
{
    Q_ASSERT(resolution.isValid());
    if (!resolution.isValid()) {
        return;
    }

    Q_ASSERT(roi.width < int(resolution.getWidth()));
    Q_ASSERT(roi.height < int(resolution.getHeight()));

    double width = resolution.getWidth();
    double height = resolution.getHeight();
    m_roi = QRectF(roi.x/width, roi.y/height, roi.width/width, roi.height/height);
}

bool Resolution::fromString(const QString &resolution)
{
    //remove spaces
    QString resolutionString = resolution.simplified();
    //split at x
    QStringList xSplitList = resolutionString.split(stringContainer::ROISpliter);

    int width = -1;
    int height = -1;
    if(xSplitList.size() <= 1) {
        //we dont have a x to split between
    }
    else {
        bool oneInteger = false;

        //DETERMINE Width
        //create space split list
        QStringList spaceSplitList = xSplitList[0].split(" ");
        //iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            //remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            //parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    width = parseTemp;
                }
                else {
                    //more than one number in one x-section
                    return false;
                }
            }
        }
        //catch if no number is inside x String
        if (!oneInteger){
            return false;
        }
        oneInteger = false;
        //DETERMINE Height
        //create space split list
        spaceSplitList = xSplitList[1].split(" ");
        //iterate over x split Strings which are split by a space
        for (int n = 0; n < spaceSplitList.size(); n++) {
            //remove all but numbers in string
            spaceSplitList[n].replace(QRegExp("[^\\d]"), "");
            //parse the leftover String
            int parseTemp = spaceSplitList[n].toInt();
            if (parseTemp > 0) {
                if (!oneInteger) {
                    oneInteger = true;
                    height = parseTemp;
                }
                else {
                    //more than one number in one y-section
                    return false;
                }
            }
        }
        //catch if no number is inside x String
        if (!oneInteger){
            return false;
        }
    }
    if (width< 0 || height < 0) return false;

    m_width = uint(width);
    m_height = uint(height);
    return true;
}
