#ifndef DISTRIBUTIONSELECTOR_H
#define DISTRIBUTIONSELECTOR_H

#include <QObject>

#include "keyframeselector.h"
#include "factory.h"

#define THRESHOLD_NAME "Threshold"
#define THRESHOLD_DESCRIPTION "A resolution dependend threshold, that specifies when there was enough movement to set a new keyframe."


/**
 * @class DistributionSelector
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The DistributionSelector class implements the KeyframeSelector interface.
 *        It selects frames in such a way, that they are distirubted in evenly regarding the flow.
 *        So if the camera movement is higher there will be more keyframes.
 *
 * @date 2022/07/18
 */
class DistributionSelector : public KeyframeSelector
{
    Q_OBJECT
public:
    explicit DistributionSelector(Settings settings);
    /**
     * @brief select picks keyframes from the given frameVector
     * @param frameVector holds indicies which can be picked
     * @param flowValues holds flow values corresponding to the frameIndicies,
     *        which are specified in the frameVecotr
     *        flowValue[i] is the value between the frames with the inidicies frameVector[i] and frameVector[i+1]
     * @param stopped
     * @return selected keyframes in the form of inidicies
     */
    std::vector<uint> select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped) override;

private:
    // functions
    void readSettings(Settings settings);
    // member variables
    double m_threshold = 2.0;
};

static KeyframeSelector::Settings settings = {
    KeyframeSelector::Parameter{THRESHOLD_NAME, THRESHOLD_DESCRIPTION, (double)2.0}
};

REGISTER_SELECTOR("distribution selector", DistributionSelector, settings)

#endif // DISTRIBUTIONSELECTOR_H
