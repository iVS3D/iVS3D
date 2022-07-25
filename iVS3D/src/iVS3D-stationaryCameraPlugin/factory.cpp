#include "factory.h"

std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>
Factory::createComponents(std::vector<uint> futureFrames,
                          Reader *reader,
                          double downSampleFactor,
                          bool useCuda,
                          QString selectorName,
                          KeyframeSelector::Settings selectorSettings)
{
    ImageGatherer *ig = createImageGatherer(futureFrames, reader, downSampleFactor, useCuda);
    FlowCalculator *fc = createFlowCalculator(useCuda);
    KeyframeSelector *ks = createKeyframeSelector(selectorName, selectorSettings);
    return std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>(ig, fc, ks);
}

QMap<QString, KeyframeSelector::Settings> Factory::getAllSelectors()
{
    // returns the m_available map without the AbstractKeyframeSelector entry
    QMap<QString, KeyframeSelector::Settings> selectorPairList;
    for (QString name : m_availableSelectors.keys()) {
        selectorPairList.insert(name, m_availableSelectors.value(name).second);
    }
    return selectorPairList;
}

bool Factory::reg(QString selectorName, AbstractKeyframeSelector builder, KeyframeSelector::Settings selectorSettings)
{
    auto iter = m_availableSelectors.insert(selectorName, QPair<AbstractKeyframeSelector, KeyframeSelector::Settings>(builder, selectorSettings));
    return iter != m_availableSelectors.end();
}

Factory::Factory()
{

}

ImageGatherer *Factory::createImageGatherer(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda)
{
    if (useCuda) {
#ifdef WITH_CUDA
        return new ImageGathererCuda(reader, downSampleFactor, futureFrames);
#endif
    } else {
        return new ImageGathererCpu(reader, downSampleFactor, futureFrames);
    }
}

FlowCalculator *Factory::createFlowCalculator(bool useCuda)
{
    if (useCuda) {
#ifdef WITH_CUDA
        return new FlowCalculatorCuda();
#endif
    } else {
        return new FlowCalculatorCpu();
    }
}

KeyframeSelector *Factory::createKeyframeSelector(QString name, KeyframeSelector::Settings settings)
{
    AbstractKeyframeSelector abstractSelector = m_availableSelectors.find(name)->first;
    return abstractSelector(settings);
}
