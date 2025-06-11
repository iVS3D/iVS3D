/**
 * @file reduceWithIndex.cpp
 * @brief Example usage of the Tensor class to create a 3D tensor and reduce it along an axis by finding the index of the maximum value.
 * 
 * @details This example demonstrates how to create a 3D tensor, reduce it along a specified axis using the ReduceArgMax operation,
 * and print the reduced values. The 3D tensor is initialized with some sample data, and the reduction operation finds the index of the maximum value along the specified axis.
 * The output shows the original tensor, the reduced tensor, and the values of the squeezed tensor.
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include <Tensor.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>

int main() {
    using namespace NN;

    // Create a 3D tensor with float data, i.e. per pixel class scores from semantic segementation
    Shape shape3d = {2, 3, 2}; // 2x3x2 tensor (CHW format)
    std::vector<float> data3d = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0,
                                 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};
    auto tensor3d = Tensor::fromData(std::move(data3d), shape3d);
    if(!tensor3d) {
        std::cout << "Failed to create 3D tensor: " << tensor3d.error();
        return 1;
    }
    std::cout << "3D Tensor: " << tensor3d.value().toString() << std::endl;

    // Reduce with index along axis 0 (i.e. per pixel)
    auto reduced_tensor3d = tensor3d.value().reduceWithIndex(ReduceArgMax{}, 0);
    if(!reduced_tensor3d) {
        std::cout << "Failed to reduce 3D tensor: " << reduced_tensor3d.error();
        return 1;
    }
    std::cout << "Reduced 3D tensor: " << reduced_tensor3d.value().toString() << std::endl;

    // Squeeze the reduced tensor to remove axis with dimension 1. This happens inplace.
    auto result = reduced_tensor3d.value().squeeze();
    if(!result) {
        std::cout << "Failed to squeeze reduced tensor: " << result.error();
        return 1;
    }
    std::cout << "Squeezed tensor: " << reduced_tensor3d.value().toString() << std::endl;

    // Print values of the squeezed tensor
    auto values = reduced_tensor3d.value().toVector<int64_t>();
    if(!values) {
        std::cout << "Failed to convert squeezed tensor to vector: " << values.error();
        return 1;
    }
    std::cout << "Squeezed tensor values: ";
    for (const auto& val : values.value()) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    return 0;
}