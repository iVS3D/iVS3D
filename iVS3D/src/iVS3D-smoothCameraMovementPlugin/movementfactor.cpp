#include "movementfactor.h"

MovementFactor::MovementFactor(Settings settings)
{
    readSettings(settings);
}

std::vector<uint> MovementFactor::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped)
{
    std::vector<uint> keyframes = { frameVector[0] };
    const double maxFlow = *std::max(flowValues.begin(), flowValues.end());
    const double minFlow = *std::min(flowValues.begin(), flowValues.end());
    const uint maxSectionSize = m_sectionSize.first;
    const uint minSectionSize = m_sectionSize.second;

    // create sections
    uint currSectionStartIdx = 0;
    double oldFlowValue = 0.0;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
        double currFlowValue = flowValues[flowValuesIdx];
        double currFactor = currFlowValue / oldFlowValue;
        if (currFactor >= m_acceleration || (1.0 / currFactor) >= m_acceleration) {
            // caluclate keyframeCount
            double sectionFlow = flowValues[(currSectionStartIdx + flowValuesIdx) / 2];
            uint speedKeyframeCount = (maxFlow - minFlow) / (double)(maxSectionSize - minSectionSize) * sectionFlow + minSectionSize;
            uint accKeyframeCount = 0; // TODO
            uint sectionKeyframeCount = speedKeyframeCount + accKeyframeCount;
            // set keyframes
            uint steps = (flowValuesIdx - currSectionStartIdx) / sectionKeyframeCount; // current position is end of section
            steps = steps < 1 ? 1 : steps;
            for (uint idx = currSectionStartIdx; idx < flowValuesIdx; idx+=steps) {
                keyframes.push_back(frameVector[idx+1]);
            }
            qDebug() << "[" << frameVector[currSectionStartIdx+1] << ", " << frameVector[flowValuesIdx+1] << "] " << currFactor;

            currSectionStartIdx = flowValuesIdx;
        }
        oldFlowValue = currFlowValue;
    }
    return keyframes;
}

void MovementFactor::readSettings(Settings settings)
{
    for (auto param : qAsConst(settings)) {
        if (param.name == SECTIONSIZEMAX_NAME)
            m_sectionSize.second = param.value.toInt();
        if (param.name == SECTIONSIZEMIN_NAME)
            m_sectionSize.first = param.value.toInt();
        if (param.name == ACCELERATION_NAME)
            m_acceleration = param.value.toDouble();
    }
}
