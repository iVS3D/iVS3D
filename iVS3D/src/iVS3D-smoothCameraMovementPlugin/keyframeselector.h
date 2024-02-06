#ifndef KEYFRAMESELECTOR_H
#define KEYFRAMESELECTOR_H

#include <QObject>

/**
 * @class KeyframeSelector
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The ImageGatherer class is an interface which defines the structure for the
 *        usecase specific algorithm. The algorithm uses a vector of flow values and indicies
 *        as parameters and selects based on these frames.
 *
 * @author Dominic Zahn
 *
 * @date 2022/07/08
 */
class KeyframeSelector : public QObject
{
    Q_OBJECT
public:
    KeyframeSelector(double threshold=2.0);

    std::vector<uint> select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped);

private:
    // member variables
    double m_threshold = 2.0;
};

#endif // KEYFRAMESELECTOR_H
