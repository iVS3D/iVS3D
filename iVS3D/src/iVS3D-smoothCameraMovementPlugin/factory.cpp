#include "factory.h"

std::tuple<ImageGatherer*, FlowCalculator*, KeyframeSelector*>
Factory::createComponents(std::vector<uint> futureFrames, iReader* reader,
                          bool useCuda, double threshold) {
    ImageGatherer* ig = createImageGatherer(futureFrames, reader, useCuda);
    FlowCalculator* fc = createFlowCalculator(useCuda);
    KeyframeSelector* ks = new KeyframeSelector(threshold);
    return std::tuple<ImageGatherer*, FlowCalculator*, KeyframeSelector*>(
        ig, fc, ks);
}

Factory::Factory() {}

ImageGatherer* Factory::createImageGatherer(std::vector<uint> futureFrames,
                                            iReader* reader, bool useCuda) {
    if (useCuda) {
#ifdef WITH_CUDA
        return new ImageGathererCuda(reader, futureFrames);
#endif
        return nullptr;
    } else {
        return new ImageGathererCpu(reader, futureFrames);
    }
}

FlowCalculator* Factory::createFlowCalculator(bool useCuda) {
    if (useCuda) {
#ifdef WITH_CUDA
        return new FlowCalculatorCuda();
#endif
        return nullptr;
    } else {
        return new FlowCalculatorCpu();
    }
}
