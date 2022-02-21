#ifndef FARNEBACKOPTFLOWCPU_H
#define FARNEBACKOPTFLOWCPU_H

#include "farnebackoptflow.h"
#include <opencv2/video/tracking.hpp>
#include <QDebug>

/**
 * @class FarnebackOptFlowCPU
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The FarnebackOptFlowCPU class implements the interface FarnebackOptFlow.
 * It defines the specific methods for executing the Farneback Algorithm on CPU.
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/10
 */
class FarnebackOptFlowCPU : public FarnebackOptFlow
{
public:
    FarnebackOptFlowCPU();
    ~FarnebackOptFlowCPU() override;
    /**
     * @brief setup creates a new FarnebackOpticalflow object and checks if the creation was successfull
     * @param numLevels number of pyramid layers including the initial image; levels=1 means that no extra layers are created and only the original images are used
     * @param pyrScale parameter, specifying the image scale (<1) to build pyramids for each image; pyr_scale=0.5 means a classical pyramid, where each next layer is twice smaller than the previous one.
     * @param fastPyramids uses a faster calculation for
     * @param winSize averaging window size; larger values increase the algorithm robustness to image noise and give more chances for fast motion detection, but yield more blurred motion field.
     * @param numIters number of iterations the algorithm does at each pyramid level
     * @param polyN size of the pixel neighborhood used to find polynomial expansion in each pixel; larger values mean that the image will be approximated with smoother surfaces, yielding more robust algorithm and more blurred motion field, typically poly_n =5 or 7.
     * @param polySigma standard deviation of the Gaussian that is used to smooth derivatives used as a basis for the polynomial expansion; for poly_n=5, you can set poly_sigma=1.1, for poly_n=7, a good value would be poly_sigma=1.5.
     * @param flags - OPTFLOW_USE_INITIAL_FLOW uses the input flow as an initial flow approximation.
     *              - OPTFLOW_FARNEBACK_GAUSSIAN uses the Gaussian \texttt{winsize}\times\texttt{winsize} filter instead of a box filter of the same size for optical flow estimation
     * @return true if the setup was successfull
     */
    bool setup(int numLevels=5, double pyrScale=0.5, bool fastPyramids=false, int winSize=13, int numIters=10, int polyN=5, double polySigma=1.1, int flags=0) override;
    /**
     * @brief calculateFlow computes a flow matrix, which represents a movment between the two given images
     * @param from is the image before the movement happens
     * @param to is the image after the movement has taken place
     * @param flow is the matrix in which the flow gets stored
     * @param downSampleFactor defines about which factor the images will get downsampled before computation
     * @return true if the caculation was successfull
     */
    bool calculateFlow(const cv::Mat &from, const cv::Mat &to, cv::Mat &flow, double downSampleFactor) override;

private:
    cv::Ptr<cv::FarnebackOpticalFlow> m_farn;
    bool m_isSetup = false;
};

#endif // FARNEBACKOPTFLOWCPU_H
