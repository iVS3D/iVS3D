#include <Tensor.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>

int main() {
    using namespace NN;

    // Create a 2D tensor with integer data
    Shape shape2d = {3, 4};
    std::vector<int64_t> data2d = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12
    };

    auto tensor2d = Tensor::fromData(std::move(data2d), shape2d);
    if (!tensor2d) {
        std::cout << "Failed to create 2D tensor: " << tensor2d.error();
        return 1;
    }
    std::cout << "2D Tensor [HW]: " << tensor2d.value().toString() << std::endl;

    // Map even numbers to red (255,0,0) and odd numbers to green (0,255,0)
    auto colorized = tensor2d.value().map([](int64_t value) -> std::array<uint8_t, 3> {
        if (value % 2 == 0) {
            return {255, 0, 0}; // Red for even numbers
        } else {
            return {0, 255, 0}; // Green for odd numbers
        }
    }, 0); // Insert new axis at 0

    if (!colorized) {
        std::cout << "Failed to colorize tensor: " << colorized.error();
        return 1;
    }
    std::cout << "Colorized tensor [CHW]: " << colorized.value().toString() << std::endl;

    auto colors = colorized.value().toVector<uint8_t>();
    if (!colors) {
        std::cout << "Failed to convert colorized tensor to vector: " << colors.error();
        return 1;
    }

    std::cout << "Colorized tensor values (red-channel): ";
    for (int i = 0; i < colors.value().size()/3; ++i) {
        std::cout << static_cast<int>(colors.value()[i]) << " ";
    }
    std::cout << std::endl;
    return 0;
}

    