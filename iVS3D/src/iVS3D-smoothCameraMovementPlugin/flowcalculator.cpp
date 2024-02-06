#include "flowcalculator.h"

double FlowCalculator::flowMatToDouble(cv::Mat mat)
{
    // compute flow matrix to single value (median length of all flow vectors)
    std::vector<double> flowLengths;
    for (uint x = 0; x < (uint)mat.rows; x++) {
        for (uint y = 0; y < (uint)mat.cols; y++) {
            cv::Point2f flowVector = mat.at<cv::Point2f>(x, y);
            const double length = cv::norm(flowVector);
            flowLengths.push_back(length);
        }
    }
    double m = median(flowLengths);
    return m;
}

double FlowCalculator::median(std::vector<double> vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
