#include "factory.h"

std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>
Factory::createComponents(std::vector<uint> futureFrames,
                          Reader *reader,
                          double downSampleFactor,
                          bool useCuda,
                          double threshold)
{
    ImageGatherer *ig = createImageGatherer(futureFrames, reader, downSampleFactor, useCuda);
    FlowCalculator *fc = createFlowCalculator(useCuda);
    KeyframeSelector *ks = new KeyframeSelector(threshold);
    return std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>(ig, fc, ks);
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
