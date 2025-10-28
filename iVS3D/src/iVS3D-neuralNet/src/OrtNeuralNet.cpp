#include "OrtNeuralNet.h"

#include <iostream>
#include <opencv2/imgproc.hpp>

#ifdef _WIN32
#include <Windows.h> // needed to convert std::string to ORTCHAR_T* on windows
#endif


NN::OrtNeuralNet::OrtNeuralNet(const std::string& modelPath, bool useCuda, int gpuId)
  : m_env(ORT_LOGGING_LEVEL_WARNING, "OrtNeuralNet"), m_gpuId(gpuId)
{
    if (useCuda) {
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(m_sessionOptions, m_gpuId));
    } else {
        m_gpuId = -1; // No GPU used
    }

    // Sets graph optimization level
    // Available levels are
    // ORT_DISABLE_ALL -> To disable all optimizations
    // ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
    // ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
    // ORT_ENABLE_ALL -> To Enable All possible opitmizations
    m_sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, modelPath.c_str(), modelPath.size(), NULL, 0);
    std::wstring wstrPathToModel(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, modelPath.c_str(), modelPath.size(), &wstrPathToModel[0], size_needed);
    ORTCHAR_T* path = const_cast<ORTCHAR_T*>(wstrPathToModel.c_str());
#else
    ORTCHAR_T* path = const_cast<ORTCHAR_T*>(modelPath.c_str());
#endif

    m_session = Ort::Session(m_env, path, m_sessionOptions);

    auto inputCount = m_session.GetInputCount();
    auto outputCount = m_session.GetOutputCount();

    if (inputCount != 1 || outputCount != 1) {
        throw std::runtime_error("Only models with 1 input and 1 output are supported");
    }

    m_inputName = m_session.GetInputNameAllocated(0, m_allocator).get();
    m_outputName = m_session.GetOutputNameAllocated(0, m_allocator).get();

    auto inputTypeInfo = m_session.GetInputTypeInfo(0);
    auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
    m_inputShape = inputTensorInfo.GetShape();

    auto outputTypeInfo = m_session.GetOutputTypeInfo(0);
    auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
    m_outputShape = outputTensorInfo.GetShape();
}

tl::expected<NN::Tensor, NN::NeuralError> NN::OrtNeuralNet::infer(const NN::Tensor& input) {

    std::vector<int64_t> actualShape = input.shape();

    // Add batch dimension if needed
    std::optional<std::vector<int64_t>> overrideShape;
    if (actualShape.size() + 1 == m_inputShape.size()) {
        overrideShape = actualShape;
        overrideShape->insert(overrideShape->begin(), 1);
    } else if (actualShape.size() != m_inputShape.size()) {
        std::ostringstream oss;
        oss << "Input shape rank mismatch: got " << actualShape.size() << ", expected " << m_inputShape.size();
        return tl::unexpected(NeuralError(ErrorCode::InvalidArgument, oss.str()));
    }

    const std::vector<int64_t>& effectiveShape = overrideShape.has_value() ? *overrideShape : actualShape;

    // Validate dimensions (account for -1 wildcards)
    for (size_t i = 0; i < m_inputShape.size(); ++i) {
        if (m_inputShape[i] != -1 && effectiveShape[i] != m_inputShape[i]) {
            std::ostringstream oss;
            oss << "Shape mismatch at dim " << i << ": got " << effectiveShape[i] << ", expected " << m_inputShape[i];
            return tl::unexpected(NeuralError(ErrorCode::InvalidArgument, oss.str()));
        }
    }

    // Convert to Ort::Value
    auto ortInput = tensorToOrtValue(input, overrideShape);
    if (!ortInput) return tl::unexpected(ortInput.error());

    // Run inference using Ort
    const char* inputNames[] = { m_inputName.c_str() };
    const char* outputNames[] = { m_outputName.c_str() };

    try{
        auto outputs = m_session.Run(Ort::RunOptions{nullptr}, inputNames, &*ortInput, 1, outputNames, 1);
        // Convert output to Tensor
        return ortValueToTensor(outputs.front());
    } catch (const Ort::Exception& e) {
        // TODO: Not every Ort error is memory related!
        return tl::unexpected(NeuralError(ErrorCode::OutOfMemory, std::string("Ort exception: ") + e.what()));
    } catch (const std::exception& e) {
        return tl::unexpected(NeuralError(ErrorCode::RuntimeError, std::string("Std exception: ") + e.what()));
    }
    return tl::unexpected(NeuralError(ErrorCode::RuntimeError, "Unknown error during inference"));
}

NN::Shape NN::OrtNeuralNet::inputShape() const {
    return m_inputShape;
}

NN::Shape NN::OrtNeuralNet::outputShape() const {
    return m_outputShape;
}

int NN::OrtNeuralNet::gpuId() const
{
    return m_gpuId;
}

tl::expected<Ort::Value, NN::NeuralError> NN::OrtNeuralNet::tensorToOrtValue(const Tensor& tensor, std::optional<std::vector<int64_t>> shapeOverride) const {
    return std::visit([&](const auto& data) -> tl::expected<Ort::Value, NN::NeuralError> {
        using T = typename std::decay<decltype(data)>::type::value_type;

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::vector<int64_t> shape = shapeOverride.has_value() ? *shapeOverride : tensor.shape();

        if (tensor.numElements() != shapeNumElements(shape)) {
            return tl::unexpected(NeuralError(ErrorCode::InvalidArgument, "Shape override doesn't match total element count."));
        }

        if (shape.empty()) {
            return tl::unexpected(NeuralError(ErrorCode::InvalidArgument, "Tensor shape is empty"));
        }

        return Ort::Value::CreateTensor<T>(
            memoryInfo,
            const_cast<T*>(data.data()),
            data.size(),
            shape.data(),
            shape.size()
        );
    }, tensor.m_data);
}

tl::expected<NN::Tensor, NN::NeuralError> NN::OrtNeuralNet::ortValueToTensor(const Ort::Value& value) const {
    if (!value.IsTensor()) return tl::unexpected(NeuralError(ErrorCode::RuntimeError, "OrtValue is not a tensor"));

    auto typeInfo = value.GetTensorTypeAndShapeInfo();
    auto shape = typeInfo.GetShape();
    auto totalSize = 1;
    for (auto dim : shape) totalSize *= dim;

    auto type = typeInfo.GetElementType();

    switch (type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: {
            const float* data = value.GetTensorData<float>();
            return Tensor::fromData(std::vector<float>(data, data + totalSize), shape);
        }
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: {
            const uint8_t* data = value.GetTensorData<uint8_t>();
            return Tensor::fromData(std::vector<uint8_t>(data, data + totalSize), shape);
        }
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: {
            const int64_t* data = value.GetTensorData<int64_t>();
            return Tensor::fromData(std::vector<int64_t>(data, data + totalSize), shape);
        }
        default:
            return tl::unexpected(NeuralError(ErrorCode::RuntimeError, "Unsupported output tensor type"));
    }
}