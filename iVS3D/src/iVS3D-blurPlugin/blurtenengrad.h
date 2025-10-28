#ifndef BLURTENENGRAD_H
#define BLURTENENGRAD_H

#include "BlurAlgorithm.h"
#include <opencv2/core.hpp>
#include <QString>

class BlurTenengrad : public BlurAlgorithm
{
public:
    BlurTenengrad();

    QString getName() override;

    void setEdgeThreshold(double t) { m_edgeThreshold = t; }
protected:
    double singleCalculation(const cv::Mat &image) override;
private:
    // Gradient magnitude threshold in 8-bit intensity units
    double m_edgeThreshold = 15.0;
};

#endif // BLURTENENGRAD_H
