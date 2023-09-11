#ifndef ANKERSPACESELECTOR_H
#define ANKERSPACESELECTOR_H

#include <QObject>
#include <Map.h>

#include "keyframeselector.h"
#include "factory.h"

#define FRAMEREDUCTION_NAME QObject::tr("Frame reduction")
#define FRAMEREDUCTION_DESCRIPTION QObject::tr("Describes how many frames should be removed. (0.00=none, 1.00=all, -N=disabled). If this parameter is set the threshold used as start value for the binary search.")
#define ANKER_TO_FILL_RATIO 5 // every xth selected keyframe is an anker frame

/**
 * @class AnkerspaceSelector
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The AnkerSpaceSelector class implements the keyframeSelector interface.
 *        To select Keyframes it starts with putting down anker frames and than filling
 *        them up with new Keyframes based on the movement between those anker frames.
 *
 * @date 2023/09/08
 *
 * @author Dominic Zahn
 */
class AnkerSpaceSelector : public KeyframeSelector {
    Q_OBJECT
public:
    explicit AnkerSpaceSelector(Settings settings);
    /**
     * @brief select picks keyframes from the given frameVector
     * @param frameVector holds indicies which can be picked
     * @param flowValues holds flow values corresponding to the frameIndicies,
     *        which are specified in the frameVecotr
     *        flowValue[i] is the value between the frames with the inidicies frameVector[i] and frameVector[i+1]
     * @param stopped
     * @return selected keyframes in the form of inidicies
     */
    std::vector<uint> select(std::vector<uint> frameVector,
                             std::vector<double> flowValues,
                             volatile bool *stopped) override;
private:
    // functions
    void readSettings(Settings settings);
    std::vector<uint> nthFrameSelection(std::vector<uint> inputIdx, int N);
    void registerKeySpace(std::multimap<double,std::pair<uint,uint>> &keySpaceSpaceMap,
                          std::vector<uint> flowIdxs,
                          std::vector<double> flowValues,
                          uint idx_begin,
                          uint idx_end);
    double movementFromKeySpace(std::pair<uint,uint> keyframeSpace,
                               std::vector<uint> &flowIdxs,
                               std::vector<double> &flowValues);
    uint selectKeyframeFromSpace(std::pair<uint,uint> keyframeSpace,
                                 std::vector<uint> &flowIdxs,
                                 std::vector<double> &flowValues);

    // member variables
    double m_frameReduction = 0.90;
};

static KeyframeSelector::Settings settings = {
    KeyframeSelector::Parameter{FRAMEREDUCTION_NAME, FRAMEREDUCTION_DESCRIPTION, (double)0.90}
};

REGISTER_SELECTOR("Ankerspace Selector", AnkerSpaceSelector, settings)

#endif // ANKERSPACESELECTOR_H
