#include "factory.h"

Factory::Factory(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda)
{
    setup(futureFrames, reader, downSampleFactor, useCuda);
}

void Factory::setup(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda)
{
    m_futureFrames = futureFrames;
    m_reader = reader;
    m_downSampleFactor = downSampleFactor;
    m_type = useCuda ? CUDA : CPU;

    // create new hardware specific object because the parameters may changed
    if (m_specificFlowCalculator) {
        delete m_specificFlowCalculator;
        m_specificFlowCalculator = nullptr;
    }
    if (m_specificImageGatherer) {
        delete m_specificImageGatherer;
        m_specificImageGatherer = nullptr;
    }
    m_specificFlowCalculator = createFlowCalculator(m_type);
    m_specificImageGatherer = createImageGatherer(m_type);
}

ImageGatherer *Factory::getImageGatherer()
{
    if (!m_specificImageGatherer) {
        m_specificImageGatherer = createImageGatherer(m_type);
    }
    return  m_specificImageGatherer;
}

FlowCalculator *Factory::getFlowCalculator()
{
    if (!m_specificFlowCalculator) {
        m_specificFlowCalculator = createFlowCalculator(m_type);
    }
    return m_specificFlowCalculator;
}

ImageGatherer *Factory::createImageGatherer(int type)
{
    if (type == CPU) {
        return new ImageGathererCpu(m_reader, m_downSampleFactor, m_futureFrames);
    } else if (type == CUDA) {
#ifdef WITH_CUDA
        return new ImageGathererCuda(m_reader, m_downSampleFactor, m_futureFrames);
#endif
    } else {
        return nullptr;
    }
}

FlowCalculator *Factory::createFlowCalculator(int type)
{
    if (type == CPU) {
        return new FlowCalculatorCpu();
    } else if (type == CUDA) {
#ifdef WITH_CUDA
        return new FlowCalculatorCuda();
#endif
    } else {
        return nullptr;
    }
}
