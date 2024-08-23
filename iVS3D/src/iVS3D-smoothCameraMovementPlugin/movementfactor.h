#ifndef MOVEMENTFACTOR_H
#define MOVEMENTFACTOR_H

#include <QObject>

#include "keyframeselector.h"
#include "factory.h"

#define SECTIONSIZEMAX_NAME "Maximum section size"
#define SECTIONSIZEMAX_DESCRIPTION "Defines the maximum size in frames that a section can be."
#define SECTIONSIZEMIN_NAME "Minimum section size"
#define SECTIONSIZEMIN_DESCRIPTION "Defines the maximum size in frames that a section can be."
#define ACCELERATION_NAME "Acceleration"
#define ACCELERATION_DESCRIPTION "Frame pairs which have a lower acceleration between them than this factor will be grouped together as a section. The acceleration facotr of 1.0 represents no accleration."

/**
 * @class MovementFactor
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The MovementFactor class implements the KeyframeSelector interface.
 *        It selects frames based on the increase in flow to the last image pair.
 *
 * @date 2022/07/21
 */
class MovementFactor : public KeyframeSelector
{
    Q_OBJECT
public:
    explicit MovementFactor(Settings settings);
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
    QPair<int, int> m_sectionSize = {0, 0};
    double m_acceleration = 1.0;
    // functions
    void readSettings(Settings settings);
};

static KeyframeSelector::Settings settings = {
    KeyframeSelector::Parameter{SECTIONSIZEMIN_NAME, SECTIONSIZEMIN_DESCRIPTION, 0},
    KeyframeSelector::Parameter{SECTIONSIZEMAX_NAME, SECTIONSIZEMAX_DESCRIPTION, 10},
    KeyframeSelector::Parameter{ACCELERATION_NAME, ACCELERATION_DESCRIPTION, (double)1.5}
};

REGISTER_SELECTOR("MovementFactor selector", MovementFactor, settings)

#endif // MOVEMENTFACTOR_H
