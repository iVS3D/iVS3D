#include "roi.h"
#include <QRegExp>

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
