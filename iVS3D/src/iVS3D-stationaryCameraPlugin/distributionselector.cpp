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

    std::vector<uint> keyframes = { frameVector[0] };
    double flowSum = 0.0;
    for (uint flowValuesIdx = 0; flowValuesIdx < flowValues.size() - 1; flowValuesIdx++) {
        flowSum += flowValues[flowValuesIdx];
        // ----------- selection -----------
        if (flowSum >= m_threshold) {
            flowSum = 0.0;
            keyframes.push_back(frameVector[flowValuesIdx + 1]);
        }
    }
    return keyframes;
}

void DistributionSelector::readSettings(Settings settings)
{
    m_threshold = settings.first().value.toDouble();
}
