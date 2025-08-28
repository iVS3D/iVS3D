#pragma once

#include "BlurAlgorithm.h"
#include <opencv2/core.hpp>
#include <QString>

class BlurTenengrad : public BlurAlgorithm
{
public:
    BlurTenengrad();

    QString getName() override;
    double singleCalculation(const cv::Mat &image) override;

    void setEdgeThreshold(double t) { m_edgeThreshold = t; }

private:
    // Gradient magnitude threshold in 8-bit intensity units
    double m_edgeThreshold = 15.0;
};
