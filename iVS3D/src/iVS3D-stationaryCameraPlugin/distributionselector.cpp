#include "distributionselector.h"

DistributionSelector::DistributionSelector(Settings settings)
{
    readSettings(settings);
}

std::vector<uint> DistributionSelector::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped)
{
    if (frameVector.size() < 1) {
        return {};
    }

    uint targetFrameCount = frameVector.size() * (1.0 - m_frameReduction);
    float min_threshold = 0;
    float max_threshold = 10000;
    std::vector<uint> keyframes = {};
    uint keyframeCount = 0;
    do {
        if (*stopped)
            return keyframes;

        keyframes = selectWithThreshold(frameVector, flowValues, stopped);
        keyframeCount = keyframes.size();

        std::cout << std::to_string(m_threshold) << "[" << std::to_string(min_threshold) << ", " << std::to_string(max_threshold)
                  << "] count: " << std::to_string(keyframeCount) << "(" << std::to_string(targetFrameCount) << ")\n";

        if (keyframeCount < targetFrameCount) {
            // increase frame count and decrease threshold
            max_threshold = m_threshold;
            m_threshold = min_threshold + (m_threshold-min_threshold)/2;
        } else {
            // deacrease frame count and increase threshold
            min_threshold = m_threshold;
            m_threshold = m_threshold + (max_threshold-m_threshold)/2;
        }
    } while (keyframeCount != targetFrameCount);
    return keyframes;
}

void DistributionSelector::readSettings(Settings settings)
{
    for (auto s : settings) {
        if (s.name == THRESHOLD_NAME)
            m_threshold = s.value.toDouble();
        if (s.name == FRAMEREDUCTION_NAME)
            m_frameReduction = s.value.toDouble();
    }
}

std::vector<uint> DistributionSelector::selectWithThreshold(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped)
{
    std::vector<uint> keyframes = { frameVector[0] };
    double flowSum = 0.0;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
        if (*stopped)
            return keyframes;
        flowSum += flowValues[flowValuesIdx];
        // ----------- selection -----------
        if (flowSum >= m_threshold) {
            flowSum = 0.0;
            keyframes.push_back(frameVector[flowValuesIdx + 1]);
        }
    }
    return keyframes;
}
