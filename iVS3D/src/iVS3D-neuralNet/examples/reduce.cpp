/**
 * @file reduce.cpp
 * @brief Example usage of the Tensor class to create a 2D tensor and reduce it along an axis by summing the values.
 *
 * @details This example demonstrates how to create a 2D tensor, reduce it along a specified axis using the ReduceSum operation,
 * and print the reduced values.
 * The 2D tensor is initialized with some sample data, and the reduction operation sums the values along the specified axis.
 * The output shows the original tensor, the reduced tensor, and the values of the reduced tensor.
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include <Tensor.h>
#include <ReduceOps.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>

int main() {
    using namespace NN;

    // Create a 2D tensor with float data
    Shape shape2d = {3, 4}; // 3x4 tensor (CHW format)
    std::vector<float> data2d = {1.0, 2.0, 3.0, 4.0,
                                 5.0, 6.0, 7.0, 8.0,
                                 9.0, 10.0, 11.0, 12.0};
    auto tensor2d = Tensor::fromData(std::move(data2d), shape2d);
    if(!tensor2d) {
        std::cout << "Failed to create 2D tensor: " << tensor2d.error();
        return 1;
    }
    std::cout << "2D Tensor: " << tensor2d.value().toString() << std::endl;

    // Reduce the 2D tensor along axis 1 (i.e. sum across rows)
    auto reduced_tensor2d = tensor2d.value().reduce(ReduceSum{}, 1);
    if(!reduced_tensor2d) {
        std::cout << "Failed to reduce 2D tensor: " << reduced_tensor2d.error();
        return 1;
    }
    std::cout << "Reduced 2D Tensor: " << reduced_tensor2d.value().toString() << std::endl;

    std::cout << "Reduced values: ";
    auto values = reduced_tensor2d.value().toVector<float>();
    if(!values) {
        std::cout << "Failed to convert reduced tensor to vector: " << values.error();
        return 1;
    }
    for (const auto& val : values.value()) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    // The reduced values are the sums of each row:
    // Row 0: 1.0 + 2.0 + 3.0 + 4.0 = 10.0
    // Row 1: 5.0 + 6.0 + 7.0 + 8.0 = 26.0
    // Row 2: 9.0 + 10.0 + 11.0 + 12.0 = 42.0
    return 0;
}