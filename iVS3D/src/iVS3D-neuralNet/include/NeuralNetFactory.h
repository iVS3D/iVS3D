#pragma once

#include "NeuralNet.h"

#include <memory>
#include <string>
#include <iostream>
#include <tl/expected.hpp>

namespace NN
{
    class NeuralNetFactory
    {
    public:
        static tl::expected<NeuralNetPtr, std::string> create(const std::string& modelPath, bool useCuda = false, int gpuId = 0);
    };
}