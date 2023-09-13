#include "flowcalculator.h"

void FlowCalculator::logDebugInfo(LogFileParent *logFile)
{
    std::stringstream ax_stream, ay_stream, cs_stream;
    for (int i = 0; i < (int)m_axs.size(); i++) {
        ax_stream << i << "|" << m_axs[i];
        ay_stream << i << "|" << m_ays[i];
        cs_stream << i << "|" << m_cs[i];
        if (i < (int)m_axs.size()-1) {
            ax_stream << ",";
            ay_stream << ",";
            cs_stream << ",";
        }
    }

    logFile->addCustomEntry(LF_AXS, QVariant(QString::fromStdString(ax_stream.str())));
    logFile->addCustomEntry(LF_AYS, QVariant(QString::fromStdString(ay_stream.str())));
    logFile->addCustomEntry(LF_CS, QVariant(QString::fromStdString(cs_stream.str())));
}

double FlowCalculator::flowMatToDouble(cv::Mat mat)
{
    const uint w = mat.size().width;
    const uint h = mat.size().height;
    //       fit graph
    /* setup A, v, b for A*v=b
     *      __                 __                    --    --
     *     | gx(0,w)  gy(0,h)  1 |                  | f(0,0) |
     *     | gx(1,w)  gy(0,h)  . |      -    -      | f(1,0) |
     *     |   .        .      . |     |  ax  |     |    .   |
     * A = |   .        .      . | v = |  ay  | b = |    .   |
     *     | gx(w,w)  gy(0,h)  1 |     |  c   |     | f(w,0) |
     *     | gx(0,w)  gy(1,h)  1 |      -    -      | f(0,1) |
     *     |   .        .      . |                  |    .   |
     *     |   .        .      . |                  |    .   |
     *     | gx(w,w)  gy(h,h)  1 |                  | f(w,h) |
     *      --                 --                    --    --
     */
    Eigen::MatrixXd A(w*h,3);
    Eigen::VectorXd b(w*h);
    for (uint y = 0; y < h; y++) {
        for (uint x = 0; x < w; x++) {
            double f = cv::norm(mat.at<cv::Point2f>(y,x)); // flow magnitude
            b[y*w+x] = f;
            A(y*w+x, 0) = gx(x,w);
            A(y*w+x, 1) = gy(y,h);
            A(y*w+x, 2) = 1;
        }
    }
    // solving linear least squares system
    Eigen::Vector3d v = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    if (std::isnan(v(0)) || std::isnan(v(1)) || std::isnan(v(2))) {
        std::atomic<uint>(m_failCounter++);
        std::cout << m_failCounter << std::endl;
    }

    /*      calculate movement from graph parameters
     *   c   = base movement strength
     * tr_x,tr_y = translation-to-rotation ratio (0: rotation, 1: translation),
     *         translatation:  0 - 10 => 1.0
     *         rotation:      10 - 20 => 1.0 - 0.0
    */
    double tr_x = aiToTr(v(0));
    double tr_y = aiToTr(v(1));
    double c = v(2);

    // DEBUG
    m_axs.push_back(v[0]);
    m_ays.push_back(v[1]);
    m_cs.push_back(v[2]);
    //

    return c * tr_x * tr_y;
}

double FlowCalculator::median(std::vector<double> vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}

double FlowCalculator::gx(uint x, uint w)
{
    double x_n = (double)x / (w-1);
    return pow(((double)x_n-0.5),2);
}

double FlowCalculator::gy(uint y, uint h)
{
    double y_n = (double)y / (h-1);
    return pow(((double)y_n-0.5),2);
}

double FlowCalculator::aiToTr(double ai)
{
    if (ai <= TR_MIN)
        return 1.0;
    else if (TR_MIN < ai && ai <= TR_MAX)
        return 1.0 - (ai-TR_MIN) / (TR_MAX-TR_MIN);
    else
        return 0.0;
}
