#ifndef FARNEBACKOPTFLOWFACTORY_H
#define FARNEBACKOPTFLOWFACTORY_H

#include "farnebackoptflow.h"
#include "farnebackoptflowcpu.h"

#ifdef WITH_CUDA
#include "farnebackoptflowgpu.h"
#endif // WITH_CUDA

#define FARNEBACK_CUDA 1
#define FARNEBACK_CPU 0

/**
 * @class FarnebackOptFlowFactory
 *
 * @ingroup CameraMovementPlugin
 *
 * @brief The FarnebackOptFlowFactory creates either a FranebackOptFlow object that uses the CPU or one that utalizes the GPU.
 *
 * @author Dominik Wüst
 *
 * @date 2021/04/10
 */
class FarnebackOptFlowFactory
{
public:
    /**
     * @brief create generates a new object that implements the FarnbackOptFlow interface.
     * The function decides based on the parameter type if it should choose the CPU or GPU implementation.
     * @param type
     * @return
     */
    static FarnebackOptFlow *create(int type);
};

#endif // FARNEBACKOPTFLOWFACTORY_H
