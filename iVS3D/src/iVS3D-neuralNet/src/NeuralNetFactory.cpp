#include "NeuralNetFactory.h"
#include "OrtNeuralNet.h"

tl::expected<NN::NeuralNetPtr,std::string> NN::NeuralNetFactory::create(const std::string &modelPath, bool useCuda, int gpuId)
{
    try{
        return std::make_shared<OrtNeuralNet>(modelPath, useCuda, gpuId);
    } catch (const Ort::Exception& e) {
        return tl::unexpected(std::string("ONNX Runtime error: ") + e.what());
    } catch (const std::exception& e) {
        return tl::unexpected(std::string("Error creating OrtNeuralNet: ") + e.what());
    } catch (...) {
        return tl::unexpected("Unknown error creating OrtNeuralNet");
    }
}
