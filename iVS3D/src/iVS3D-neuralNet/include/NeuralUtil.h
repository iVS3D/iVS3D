#pragma once

#include <Tensor.h>
#include <NeuralNet.h>

namespace NN::Util {

template<typename ReduceOp>
auto bind_reduce(ReduceOp op, int axis) {
    return [=](NN::Tensor&& t){
        return t.reduce(op, axis);
    };
}

template<typename ReduceIndexOp>
auto bind_reduceWithIndex(ReduceIndexOp op, int axis) {
    return [=](NN::Tensor&& t) {
        return t.reduceWithIndex(op, axis);
    };
}

template<typename Func>
auto bind_map(Func f) {
    return [=](NN::Tensor&& t) {
        return t.map(f);
    };
}

template<typename Func>
auto bind_map(Func f, int axis) {
    return [=](NN::Tensor&& t) {
        return t.map(f, axis);
    };
}

template<typename... Args>
auto bind_reshape(const std::vector<int64_t>& newShape) {
    return [=](NN::Tensor&& t) {
        return t.reshape(newShape);
    };
}

template<typename... Args>
auto bind_squeeze() {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.squeeze();
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}


template<typename... Args>
auto bind_squeeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.squeeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

template<typename... Args>
auto bind_unsqueeze(int64_t axis) {
    return [=](NN::Tensor&& tensor) -> tl::expected<NN::Tensor,std::string> {
        auto result = tensor.unsqueeze(axis);
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::move(tensor);
    };
}

template<typename... Args>
auto bind_toCvMat() {
    return [=](NN::Tensor&& tensor) {
        return tensor.toCvMat();
    };
}


template<typename... Args>
auto bind_inference(NN::NeuralNetPtr model) {
    return [=](NN::Tensor&& input) {
        return model->infer(input);
    };
}

} // namespace NN::Util