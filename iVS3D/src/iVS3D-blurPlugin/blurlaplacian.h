#ifndef BLURLAPLACIAN_H
#define BLURLAPLACIAN_H

#include <QObject>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <QtConcurrent/QtConcurrentMap>
#include <iostream>
#include "reader.h"
#include "progressable.h"
#include "BlurAlgorithm.h"


/**
 * @class BlurLaplacian
 *
 * @ingroup BlurPlugin
 *
 * @brief The BlurLaplacian class calulates blur values based on a algotihm using the Laplacian filter https://www.researchgate.net/publication/234073157_Analysis_of_focus_measure_operators_in_shape-from-focus
 * The OpenCV implementation is provided by https://stackoverflow.com/a/7768918
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/19
 */

class BlurLaplacian : public BlurAlgorithm
{

public:
    /**
     * @brief BlurLaplacian standard constructor
     */
    BlurLaplacian();
    /**
     * @brief getName Returns the algorithm name
     * @return "Laplacian Filter"
     */
    QString getName() override;

protected:
    QString m_name = "Laplacian Filter";
    double singleCalculation(cv::Mat image) override;




};

#endif // BLURLAPLACIAN_H
