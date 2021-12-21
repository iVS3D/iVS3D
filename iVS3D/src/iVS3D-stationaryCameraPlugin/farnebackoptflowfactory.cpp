#include "farnebackoptflowfactory.h"

FarnebackOptFlow *FarnebackOptFlowFactory::create(int type)
{
    if(type == FARNEBACK_CPU){
        return new FarnebackOptFlowCPU;
    } else if (type == FARNEBACK_CUDA){
#ifdef WITH_CUDA
        return new FarnebackOptFlowGPU;
#else // !WITH_CUDA
        return new FarnebackOptFlowCPU;
#endif // WITH_CUDA
    } else {
        return nullptr;
    }
}
