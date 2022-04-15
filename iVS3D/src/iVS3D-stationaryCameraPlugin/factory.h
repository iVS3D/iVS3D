#ifndef FACTORY_H
#define FACTORY_H

#include <QObject>
#include <reader.h>

#include "imagegatherer.h"
#include "imagegatherercpu.h"
#include "flowcalculator.h"
#include "flowcalculatorcpu.h"

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
    /**
     * @brief Factory Constructors sets all parameters that are required to construct both objects
     * @param futureFrames as list of all frames that will be requested from the imageGatherer in the future
     * @param reader is the object which the imageGatherer uses to extract the images
     * @param downSampleFactor is a double value which specifies how much the input image will be sampled down
     * @param useCuda defines the implementation which should be used (CPU or CUDA)
     */
    Factory(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda);
    /**
     * @brief setup can be used to change the parameters which are required to construct both objects
     * @param futureFrames as list of all frames that will be requested from the imageGatherer in the future
     * @param reader is the object which the imageGatherer uses to extract the images
     * @param downSampleFactor is a double value which specifies how much the input image will be sampled down
     * @param useCuda defines the implementation which should be used (CPU or CUDA)
     */
    void setup(std::vector<uint> futureFrames, Reader *reader, double downSampleFactor, bool useCuda);
    /**
     * @brief getImageGatherer Getter for ImageGatherer object
     * @return Pointer to ImageGatherer object
     */
    ImageGatherer* getImageGatherer();
    /**
     * @brief getFlowCalculator Getter for FlowCalculator object
     * @return Pointer to FlowCalculator object
     */
    FlowCalculator* getFlowCalculator();

private:
    ImageGatherer* createImageGatherer(int type);
    FlowCalculator *createFlowCalculator(int type);
    // member variables
    ImageGatherer *m_specificImageGatherer = nullptr;
    FlowCalculator *m_specificFlowCalculator = nullptr;
    std::vector<uint> m_futureFrames = {};
    Reader *m_reader = nullptr;
    double m_downSampleFactor = 1.0f;
    int m_type = CPU;

};

#endif // FACTORY_H
