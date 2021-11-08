#include "itransform_stub.h"

ITransform_stub::ITransform_stub()
{
}

QString ITransform_stub::getName() const
{
    return "testITransform";
}

QStringList ITransform_stub::getOutputNames()
{
    return {"masks", "masksfancy"};
}

ImageList ITransform_stub::transform(uint, const cv::Mat &img)
{
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    return ImageList({gray, gray});
}
