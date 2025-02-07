#include "keyframeselector.h"

KeyframeSelector::KeyframeSelector(double threshold) {
    m_threshold = threshold;
}

std::vector<uint> KeyframeSelector::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped) {
    std::vector<uint> selectedKeyframes = { frameVector[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size(); flowValuesIdx++) {
        if (flowValues[flowValuesIdx] < 0.0) {
            continue;
        }
        // ----------- selection --------------
        if (flowValues[flowValuesIdx] > allowedDiffFlow) {
            selectedKeyframes.push_back(frameVector[flowValuesIdx + 1]); // flow value represents flow for the next frame (if camera moved enough until next frame)
        }
    }
    if (*stopped) {
        // clear keyframes if algorithm was aborted
        selectedKeyframes = {};
    }
    return selectedKeyframes;
}

double KeyframeSelector::median(std::vector<double> &vec) {
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
