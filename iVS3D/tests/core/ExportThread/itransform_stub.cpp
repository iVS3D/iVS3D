#include "itransform_stub.h"

ITransform_stub::ITransform_stub()
{
}

QString ITransform_stub::getName() const
{
    return "testITransform";
}

TransformResult ITransform_stub::transform(uint, const cv::Mat &img, const Resolution &resolution, const ROI &roi)
{
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    return gray;
}
