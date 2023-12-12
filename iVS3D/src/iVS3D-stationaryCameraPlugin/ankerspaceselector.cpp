#include "ankerspaceselector.h"

AnkerSpaceSelector::AnkerSpaceSelector(Settings settings) {
    readSettings(settings);
}

std::vector<uint> AnkerSpaceSelector::select(std::vector<uint> frameVector, std::vector<double> flowValues, volatile bool *stopped) {
    if (frameVector.size() < 1) {
        return {};
    }
    int targetKeyframeCount = (int)ceil((float)frameVector.size() / m_frameReduction);

    //      select anker frames
    // gather anker frames with nframe over orbKeyframes
    int ankerRatio = ANKER_TO_FILL_RATIO * m_frameReduction;
    std::vector<uint> keyframes = nthFrameSelection(frameVector, ankerRatio);

    //      setup keyframe spaces
    // map of KeyframeSpaces sorted by highest distance
    std::multimap<double,std::pair<uint,uint>> keyframeSpaceMap;
    for (int j = 1; j < (int)keyframes.size(); j++) {
        int idx_begin = keyframes[j-1];
        int idx_end = keyframes[j];
        registerKeySpace(keyframeSpaceMap, frameVector, flowValues, idx_begin, idx_end);
    }

    //      fill keyframe spaces accordingly with keyframes
    while (targetKeyframeCount > (int)keyframes.size()) {
        if (keyframeSpaceMap.empty())
            break;

        // select keyframe space with highest distance -> more keyframes neccessary
        std::pair<uint,uint> ks = keyframeSpaceMap.rbegin()->second;
        uint k_begin = ks.first;
        uint k_end = ks.second;

        // select new keyframe to insert in space
        uint k_middle = selectKeyframeFromSpace(ks, frameVector, flowValues);
        if  (k_middle == UINT_MAX) {
            // no available Keyframe in selected KeyframeSpace -> remove from posible KeyframeSpaces
            keyframeSpaceMap.erase(--keyframeSpaceMap.end());
            continue;
        }

        // split keyframe space into two new ones
        keyframeSpaceMap.erase(--keyframeSpaceMap.end());
        registerKeySpace(keyframeSpaceMap, frameVector, flowValues, k_begin, k_middle);
        registerKeySpace(keyframeSpaceMap, frameVector, flowValues, k_middle, k_end);

        // add new keyframe to index list of selected keyframes
        keyframes.push_back(k_middle);
    }

    //      bring final index list in correct order
    std::sort(keyframes.begin(), keyframes.end());
    return keyframes;
}

void AnkerSpaceSelector::readSettings(Settings settings) {
    for (auto s : settings) {
        if (s.name == FRAMEREDUCTION_NAME) {
            m_frameReduction = s.value.toInt();
        }
    }
}

std::vector<uint> AnkerSpaceSelector::nthFrameSelection(std::vector<uint> inputIdx, int N) {
    std::vector<uint> sampledIdx;
    for (int j = 0; j < (int)inputIdx.size(); j+=N) {
        sampledIdx.push_back(inputIdx[j]);
    }
    sampledIdx.push_back(inputIdx.back());
    return sampledIdx;
}

void AnkerSpaceSelector::registerKeySpace(std::multimap<double, std::pair<uint, uint> > &keySpaceSpaceMap, std::vector<uint> flowIdxs, std::vector<double> flowValues, uint idx_begin, uint idx_end) {
    std::pair<uint,uint> keyframeSpace = std::pair<uint,uint>(idx_begin,idx_end);
    double movement = movementFromKeySpace(keyframeSpace,
                                           flowIdxs,
                                           flowValues);
    keySpaceSpaceMap.insert(std::pair<double,std::pair<uint,uint>>(movement,keyframeSpace));
}

double AnkerSpaceSelector::movementFromKeySpace(std::pair<uint, uint> keyframeSpace, std::vector<uint> &flowIdxs, std::vector<double> &flowValues) {
    uint idx_begin = keyframeSpace.first;
    uint idx_end = keyframeSpace.second;

    int j_begin = std::find(flowIdxs.begin(), flowIdxs.end(), idx_begin) - flowIdxs.begin();
    int j_end = std::find(flowIdxs.begin(), flowIdxs.end(), idx_end) - flowIdxs.begin();

    // sum up all movement in keyframe space
    double movement = 0.0;
    for (int j = j_begin+1; j < j_end; j++) {
        movement += flowValues[j];
    }
    return movement;
}

uint AnkerSpaceSelector::selectKeyframeFromSpace(std::pair<uint, uint> keyframeSpace, std::vector<uint> &flowIdxs, std::vector<double> &flowValues) {
    uint k_begin = keyframeSpace.first;
    uint k_end = keyframeSpace.second;
    // create a subset of keyframes which are all inside the KeyframeSpace and calculate summed up
    // movement from the beginning to the frame (key is index in flowIdx/flowValue vector)
    std::map<uint,double> keyframeSpaceSubMap;
    double movementSum = 0.0;
    int j_begin = std::find(flowIdxs.begin(), flowIdxs.end(), k_begin) - flowIdxs.begin();
    int j_end = std::find(flowIdxs.begin(), flowIdxs.end(), k_end) - flowIdxs.begin();
    for (int j = j_begin+1; j < j_end; j++) {
        movementSum += flowValues[j];
        keyframeSpaceSubMap.insert(std::pair<uint,double>(flowIdxs[j],movementSum));
    }

    if (keyframeSpaceSubMap.empty()) {
        return UINT_MAX;
    }

    // select middle position (same distance to anker frames)
    std::map<double,uint> distToMiddleMap;
    double middleMovement = movementSum / 2;
    for (std::pair<uint,double> e : keyframeSpaceSubMap) {
        double currDistToMiddle = abs(e.second-middleMovement);
        distToMiddleMap.insert(std::pair<double,uint>(currDistToMiddle,e.first));
    }
    return distToMiddleMap.begin()->second;
}
