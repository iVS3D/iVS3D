#ifndef FLOWCALCULATOR_H
#define FLOWCALCULATOR_H

#include <QObject>
#include <opencv2/core.hpp>

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
protected:
    /**
     * @brief flowMatToDouble reduces a flow-matrix to a single floating point value
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
};

#endif // FLOWCALCULATOR_H
