#ifndef FLOWCACLULATORCUDA_H
#define FLOWCACLULATORCUDA_H

#include "flowcalculator.h"

#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaimgproc.hpp>

/**
 * @class FlowCalculatorCuda
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The FlowCalculatorCuda class is a hardware specific implementation of FlowCalculator.
 *        The algorithm takes two pictures and calculates a flow matrix using the Farneback algorithm.
 *        Than the flow matrix is evalutated and reduced to one single flow value.
 *
 * @author Dominic Zahn
 *
 * @date 2022/04/12
 */
class FlowCalculatorCuda : public FlowCalculator
{
public:
    /**
     * @brief FlowCalculatorCuda Constructor creates a Farneback object which is used to execute the farneback algortihm later
     * @param numLevels number of pyramid layers including the initial image; levels=1 means that no extra layers are created and only the original images are used
     * @param pyrScale parameter, specifying the image scale (<1) to build pyramids for each image; pyr_scale=0.5 means a classical pyramid, where each next layer is twice smaller than the previous one.
     * @param fastPyramids uses a faster calculation for
     * @param winSize averaging window size; larger values increase the algorithm robustness to image noise and give more chances for fast motion detection, but yield more blurred motion field.
     * @param numIters number of iterations the algorithm does at each pyramid level
     * @param polyN size of the pixel neighborhood used to find polynomial expansion in each pixel; larger values mean that the image will be approximated with smoother surfaces, yielding more robust algorithm and more blurred motion field, typically poly_n =5 or 7.
     * @param polySigma standard deviation of the Gaussian that is used to smooth derivatives used as a basis for the polynomial expansion; for poly_n=5, you can set poly_sigma=1.1, for poly_n=7, a good value would be poly_sigma=1.5.
     * @param flags - OPTFLOW_USE_INITIAL_FLOW uses the input flow as an initial flow approximation.
     *              - OPTFLOW_FARNEBACK_GAUSSIAN uses the Gaussian \texttt{winsize}\times\texttt{winsize} filter instead of a box filter of the same size for optical flow estimation
     */
    FlowCalculatorCuda(int numLevels=5, double pyrScale=0.5, bool fastPyramids=false, int winSize=13, int numIters=10, int polyN=5, double polySigma=1.1, int flags=0);
    /**
      * @brief FlowCalculatorCuda Destructor cleans up left over data and pointers
      */
    ~FlowCalculatorCuda();
    /**
     * @brief calculateFlow computes the flow between two images and reduces the resulting
     *        flow-matrix to a single floating point value
     * @param fromMat cv::Mat which is the image before the posible movement
     * @param toMat cv::Mat which is the image after the posible movement
     * @return doubel which represents the movement between the images
     */
    double calculateFlow(cv::Mat fromMat, cv::Mat toMat) override;

private:
    cv::Ptr<cv::cuda::FarnebackOpticalFlow> m_farn;
};

#endif // FLOWCACLULATORCUDA_H
