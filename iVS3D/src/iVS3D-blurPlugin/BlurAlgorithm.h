#ifndef BLURALGORITHM_H
#define BLURALGORITHM_H

#include <QObject>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <QtConcurrent/QtConcurrentMap>
#include "reader.h"
#include "progressable.h"

/**
 * @class BlurAlgorithm
 *
 * @ingroup BlurPlugin
 *
 * @brief The BlurAlgorithm interface provides an interface for different algorithms calculating blur values for given images.
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/19
 */

class BlurAlgorithm: public QObject
{
    Q_OBJECT

public:
    /**
     * @brief getName returns the algorithm display name.
     * @return The name
     */
    virtual QString getName() = 0;
    /**
     * @brief calcOneBluriness calculates blur meassure for one image specified by index.
     * @param images The reader to fetch image from
     * @param index The image index
     * @return The blur value
     */
    double calcOneBluriness(Reader* images, int index);
    /**
     * @brief calcFullBluriness calculates blur meassures for all images within the given range.
     * @param images The reader to fetch images from
     * @param reciever The receiver for progress updates
     * @param stopped The stopped flag @a true, if computation should abort
     * @param start index of the first image
     * @param end index of the last image
     * @param blurValues vector with the already calulated blur values (can be empty, but has the correct size)
     * @return The blur values for all images
     */
    std::vector<double> calcFullBluriness(Reader* images, Progressable* reciever, volatile bool* stopped, int start, int end, std::vector<double> blurValues);
protected:
    double parallelCalculation(cv::Mat image, std::vector<double> blurValues, int n, Progressable *receiver, int start, int picCount);
    /**
     * @brief singleCalculation This functions implements the actual blur algortihm
     * @param image cv::Mat of the image
     * @return Double represeting the blur of the image (higher value -> sharper image)
     */
    virtual double singleCalculation(cv::Mat image) = 0;
private:
    volatile int m_currentProgress = 0;


};
#endif // BLURALGORITHM_H
