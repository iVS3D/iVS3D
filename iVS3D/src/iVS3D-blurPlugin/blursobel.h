#ifndef BLURSOBEL_H
#define BLURSOBEL_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include "BlurAlgorithm.h"

/**
 * @class BlurSobel
 *
 * @ingroup BlurPlugin
 *
 * @brief The BlurSobel class calulates blur values using the sobel operator.
 * It calulates the blur values based on the Tenengrad algotihm using the sobel operator https://www.researchgate.net/publication/234073157_Analysis_of_focus_measure_operators_in_shape-from-focus
 * The OpenCV implementation is provided by https://stackoverflow.com/a/7768918
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/19
 */
class BlurSobel : public BlurAlgorithm
{

public:
    /**
     * @brief BlurSobel standard constructor
     */
    BlurSobel();
    /**
     * @brief getName returns the algorithm name.
     * @return "Sobel Operator"
     */
    QString getName() override;

protected:
    QString m_name = "Sobel Operator";
    double singleCalculation(const cv::Mat &image) override;

};

#endif // BLURSOBEL_H
