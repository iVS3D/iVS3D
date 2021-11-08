#include "blursobel.h"



BlurSobel::BlurSobel()
{

}

QString BlurSobel::getName() {
    return m_name;
}

double BlurSobel::singleCalculation(cv::Mat image)
{

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat Gx, Gy;
    cv::Sobel(image, Gx, CV_64F, 1, 0, 3);
    cv::Sobel(image, Gy, CV_64F, 0, 1, 3);

    cv::Mat FM = Gx.mul(Gx) + Gy.mul(Gy);

    double focusMeasure = cv::mean(FM).val[0];

    return focusMeasure;
}
