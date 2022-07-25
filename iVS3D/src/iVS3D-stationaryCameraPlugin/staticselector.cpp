#include "staticselector.h"

StaticSelector::StaticSelector(KeyframeSelector::Settings settings)
{
    readSettings(settings);
}

std::vector<uint> StaticSelector::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped)
{
    std::vector<uint> selectedKeyframes = { frameVector[0] };
    std::vector<double> copiedFlowValues = flowValues; // median is in place and reorders vector
    double medianFlow = median(copiedFlowValues);
    double allowedDiffFlow = medianFlow * m_threshold;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
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

void StaticSelector::readSettings(KeyframeSelector::Settings settings)
{
    m_threshold = settings.first().value.toDouble();
}

double StaticSelector::median(std::vector<double> &vec)
{
    std::vector<double>::iterator median = vec.begin() + vec.size() / 2;
    std::nth_element(vec.begin(), median, vec.end());
    return vec[vec.size() / 2];
}
