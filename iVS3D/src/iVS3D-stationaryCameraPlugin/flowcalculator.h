#ifndef FLOWCALCULATOR_H
#define FLOWCALCULATOR_H

#include <QObject>
#include <opencv2/core.hpp>
#include <eigen3/Eigen/SVD>
#include <iostream>

#include "LogFileParent.h"

// LogFile descriptor
#define LF_TAG "Debug Info"
#define LF_FAILCOUNTER "failCounter"
#define LF_AXS "axs"
#define LF_AYS "ays"
#define LF_CS "cs"

#define TR_MIN 10 // minimal value for translation-to-rotation ratio
#define TR_MAX 20 // maximal value for translation-to-rotation ratio

/**
 * @class FlowCalculator
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The FlowCalculator class is an interface which defines the structure for the
 *        hardware specific algorithm. The algorithm takes two pictures and calculates
 *        a flow matrix using the Farneback algorithm. Than the flow matrix is evalutated
 *        and reduced to one single flow value.
 *
 * @author Dominic Zahn
 *
 * @date 2022/04/12
 */
class FlowCalculator : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief calculateFlow computes the flow between two images and reduces the resulting
     *        flow-matrix to a single floating point value
     * @param fromMat cv::Mat which is the image before the posible movement
     * @param toMat cv::Mat which is the image after the posible movement
     * @return doubel which represents the movement between the images
     */
    virtual double calculateFlow(cv::Mat fromMat, cv::Mat toMat) = 0;
    void logDebugInfo(LogFileParent *logFile);
protected:
    /**
     * @brief flowMatToDouble reduces a flow-matrix to a single floating point value, if that mat was empty return -1.0
     * @param mat cv::Mat which hold represents the flow-matrix
     * @return double which represents the reduced flow-matrix
     */
    double flowMatToDouble(cv::Mat mat);
private:
    /**
     * @brief median calculates the median of a double vector in place
     * @param vec is a double vector
     * @return the median of the double vector
     */
    double median(std::vector<double> vec);
    static double gx(uint x, uint w);
    static double gy(uint y, uint h);
    static double aiToTr(double ai);

    // DEBUG
    uint m_failCounter = 0;
    std::vector<double> m_axs = {};
    std::vector<double> m_ays = {};
    std::vector<double> m_cs = {};
};

#endif // FLOWCALCULATOR_H
