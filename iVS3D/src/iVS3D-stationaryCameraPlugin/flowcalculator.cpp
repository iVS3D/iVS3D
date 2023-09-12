#include "flowcalculator.h"

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

    mat.release();

    /*      calculate movement from graph parameters
     *   c   = base movement strenght
     * ax*ay = translation-to-rotation ratio (0: rotation, 1: translation),
     *         10^6 is cut-off point between translation and rotation
    */
    double ax = v(0);
    double ay = v(1);
    double c = v(2);
    ax *= pow(10,6);
    ay *= pow(10,6);
    ax = abs(ax) > 1.0 ? 1.0 : abs(ax);
    ay = abs(ay) > 1.0 ? 1.0 : abs(ay);
    return c * ax * ay;
}

double FlowCalculator::median(std::vector<double> vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}

double FlowCalculator::graphFunc(uint x, uint y, double ax, double ay, double c, uint w, uint h) const
{
    return gx(x,w)*ax + gy(y,h)*ay + c;
}

double FlowCalculator::gx(uint x, uint w)
{
    return pow(((double)x-(double)w/2),2);
}

double FlowCalculator::gy(uint y, uint h)
{
    return pow(((double)y-(double)h/2),2);
}
