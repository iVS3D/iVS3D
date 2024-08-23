#ifndef FACTORY_H
#define FACTORY_H

#include <QObject>
#include <reader.h>
#include <tuple>

#include "imagegatherer.h"
#include "imagegatherercpu.h"
#include "flowcalculator.h"
#include "flowcalculatorcpu.h"
#include "keyframeselector.h"

#ifdef WITH_CUDA
#include "imagegatherercuda.h"
#include "flowcalculatorcuda.h"
#endif

#define CUDA 0
#define CPU 1

/**
 * @class Factory
 *
 * @ingroup StationaryCameraPlugin
 *
 * @brief The Factory class is responsible for creating and destroying as well as managing
 *        a single object of the ImageGatherer and FlowCalculator class. It creates the
 *        hardware specified type of each of these classes. (CPU or CUDA)
 *
 * @author Dominic Zahn
 *
 * @date 2022/4/5
 */
class Factory
{
public:
    static Factory &instance() {
        static Factory INSTANCE = Factory();
        return INSTANCE;
    }
    /**
     * @brief createComponents creates all neccessary components for the sample process (ImageGatherer, FlowCalculator, KeyframeSelector)
     * @param futureFrames as list of all frames that will be requested from the imageGatherer in the future
     * @param reader is the object which the imageGatherer uses to extract the images
     * @param downSampleFactor is a double value which specifies how much the input image will be sampled down
     * @param useCuda defines the implementation which should be used (CPU or CUDA)
     * @param selectorName name of the selected KeyframeSelector
     * @param selectorSettings parameters from ui, which will be used to select Keyframes
     * @return as an std::tuple containing one of each components {ImageGatherer, FlowCalculator, KeyframeSelector}
     */
    std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>
    createComponents(std::vector<uint> futureFrames,
                     Reader *reader,
                     double downSampleFactor,
                     bool useCuda,
                     double threshold);

private:
    Factory();
    ImageGatherer *createImageGatherer(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda);
    FlowCalculator *createFlowCalculator(bool useCuda);
};

#endif // FACTORY_H
