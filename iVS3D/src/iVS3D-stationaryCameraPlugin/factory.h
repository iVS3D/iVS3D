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

typedef std::function<KeyframeSelector *(KeyframeSelector::Settings)> AbstractKeyframeSelector;

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
                     QString selectorName,
                     KeyframeSelector::Settings selectorSettings);
    /**
     * @brief getAllSelectors returns a map which includes name and default settings for every available KeyframeSelector
     * @return is a QMap the name of the selector as its key, while the value represents the default settings
     */
    QMap<QString, KeyframeSelector::Settings> getAllSelectors();

    bool reg(QString selectorName, AbstractKeyframeSelector builder, KeyframeSelector::Settings selectorSettings);

private:
    Factory();
    ImageGatherer *createImageGatherer(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda);
    FlowCalculator *createFlowCalculator(bool useCuda);
    KeyframeSelector *createKeyframeSelector(QString name, KeyframeSelector::Settings settings);
    // member variables
    QMap<QString, QPair<AbstractKeyframeSelector, KeyframeSelector::Settings>> m_availableSelectors;
};

template<typename Implementaion>
KeyframeSelector *builder(KeyframeSelector::Settings settings) {
    return new Implementaion(settings);
}

#define REGISTER_SELECTOR(name, impl, settings) const bool res = Factory::instance().reg(name, builder<impl>, settings);

#endif // FACTORY_H
