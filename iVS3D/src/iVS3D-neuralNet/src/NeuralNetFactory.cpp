#include "NeuralNetFactory.h"
#include "OrtNeuralNet.h"

tl::expected<NN::NeuralNetPtr,NN::NeuralError> NN::NeuralNetFactory::create(const std::string &modelPath, bool useCuda, int gpuId)
{
    try{
        return std::make_shared<OrtNeuralNet>(modelPath, useCuda, gpuId);
    } catch (const Ort::Exception& e) {
        return tl::unexpected(NeuralError(ErrorCode::RuntimeError, std::string("ONNX Runtime error: ") + e.what()));
    } catch (const std::exception& e) {
        return tl::unexpected(NeuralError(ErrorCode::RuntimeError, std::string("Error creating OrtNeuralNet: ") + e.what()));
    } catch (...) {
        return tl::unexpected(NeuralError(ErrorCode::RuntimeError, "Unknown error creating OrtNeuralNet"));
    }
}
