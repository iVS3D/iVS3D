#include "blurlaplacian.h"



BlurLaplacian::BlurLaplacian()
{

}

QString BlurLaplacian::getName() {
    return m_name;
}


double BlurLaplacian::singleCalculation(const cv::Mat &image)
{
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat result;
    cv::Laplacian(gray, result, CV_64F,1);
    cv::Scalar mean, stddev;
    cv::meanStdDev(result, mean, stddev, cv::Mat());
    return stddev.val[0] * stddev.val[0];
}

