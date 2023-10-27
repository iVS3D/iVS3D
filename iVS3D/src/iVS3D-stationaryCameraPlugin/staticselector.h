#ifndef STATICSELECTOR_H
#define STATICSELECTOR_H

#include <QObject>

#include "keyframeselector.h"
#include "factory.h"

#define THRESHOLD_NAME QObject::tr("Threshold")
#define DESCRIPTION_THRESHOLD QObject::tr("If the rotation between two frames differs more than the defind percentage of the median rotation in the given frame sequence it is declared stationary.")

/**
 * @class StaticSelector
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The StaticSelector class implements the KeyframeSelector interface.
 *        It selects frames based on the flow value between two frames in such a way,
 *        that frames, where the camera is nearly static, are removed. The algorithm
 *        selects all frames as keyframes in the beginning of the process and than removes
 *        the stationary frames.
 *
 * @date 2022/07/11
 */
class StaticSelector : public KeyframeSelector
{
    Q_OBJECT
public:
    explicit StaticSelector(Settings settings);
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
    // member variables
    double m_threshold = 0.3;
    // functions
    void readSettings(Settings settings);
    double median(std::vector<double> &vec);
};

namespace StaticSelectorNS {
    static KeyframeSelector::Settings settings = {
        KeyframeSelector::Parameter{THRESHOLD_NAME, DESCRIPTION_THRESHOLD, (double)0.3}
    };

    REGISTER_SELECTOR("Static Selector", StaticSelector, settings)
}

#endif // STATICSELECTOR_H
