#include "keyframeselector.h"

KeyframeSelector::KeyframeSelector(double threshold) {
    m_threshold = threshold;
}

std::vector<uint> KeyframeSelector::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped) {
    if (frameVector.size() < 1) {
        return {};
    }

    std::vector<uint> keyframes = { frameVector[0] };
    double flowSum = 0.0;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size(); flowValuesIdx++) {
        if (flowValues[flowValuesIdx] < 0.0) {
            continue;
        }
        flowSum += flowValues[flowValuesIdx];
        // ----------- selection -----------
        if (flowSum >= m_threshold) {
            flowSum = 0.0;
            keyframes.push_back(frameVector[flowValuesIdx + 1]);
        }

        if (*stopped) {
            // clear keyframes if algorithm was aborted
            keyframes = {};
        }
    }
    return keyframes;
}
