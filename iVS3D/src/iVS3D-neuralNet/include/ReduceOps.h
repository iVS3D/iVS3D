#pragma once
/** @file ReduceOps.h */

#include <limits>
#include <cstddef>
#include <utility>

namespace NN {

    /**
     * @brief 
     * 
     */
// ReduceSum: computes sum along a dimension
struct ReduceSum {
    template<typename T>
    T initial() const { return T(0); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        acc += value;
    }
};

// ReduceMin: computes min value
struct ReduceMin {
    template<typename T>
    T initial() const { return std::numeric_limits<T>::max(); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        if (value < acc) acc = value;
    }
};

// ReduceMax: computes max value
struct ReduceMax {
    template<typename T>
    T initial() const { return std::numeric_limits<T>::lowest(); }

    template<typename T>
    void operator()(T& acc, const T& value) const {
        if (value > acc) acc = value;
    }
};

// ReduceArgMin: stores the index of min value
struct ReduceArgMin {
    template<typename T>
    std::pair<int64_t, T> initial() const {
        return {0, std::numeric_limits<T>::max()};
    }

    template<typename T>
    void operator()(std::pair<int64_t, T>& acc, const T& value, int64_t idx) const {
        if (value < acc.second) {
            acc = {idx, value};
        }
    }
};

// ReduceArgMax: stores the index of max value
struct ReduceArgMax {
    template<typename T>
    std::pair<int64_t, T> initial() const {
        return {0, std::numeric_limits<T>::lowest()};
    }

    template<typename T>
    void operator()(std::pair<int64_t, T>& acc, const T& value, int64_t idx) const {
        if (value > acc.second) {
            acc = {idx, value};
        }
    }
};
}