/**
 * @file inference_segmentation.cpp
 * @brief Example for performing segmentation inference using a neural network model.
 * 
 * @details This example demonstrates how to load a segmentation model, perform inference on an input image,
 * and save the segmentation result. It supports GPU inference if available.
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date May 2025
 */

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
// OpenCV is used to load and store images
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
// Our NeuralNet library for model loading and inference
#include "NeuralNetFactory.h"
#include "NeuralUtil.h"
// cxxopts for cli argument parsing
#include "cxxopts.hpp"
// Helpers for loading model configuration
#include "helpers.hpp"


/**
 * @brief Main function for segmentation inference example.
 * 
 * This example demonstrates how to load a segmentation model,
 * perform inference on an input image, and save the segmentation result.
 * It also supports GPU inference if available.
 * 
 * Usage:
 * ./inference_segmentation --model <model_path> --input <image_path> --output <output_dir> [--gpu <gpu_id>]
 * 
 * The model needs to be a segmentation model in onnx format with an argmax layer at the end.
 * The input image should be a valid image file (e.g., jpg, png).
 */
int main(int argc, char **argv)
{
    using namespace NN;

    // =========================================================
    // Command line argument parsing using cxxopts
    cxxopts::Options options(argv[0], "Segmentation inference example");
    options.add_options()
        ("m,model", "Path to model", cxxopts::value<std::string>())
        ("i,input", "Path to input image", cxxopts::value<std::string>())
        ("o,output", "Output directory", cxxopts::value<std::string>())
        ("gpu", "GPU id (-1 for CPU)", cxxopts::value<int>()->default_value("0"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help") || !result.count("model") || !result.count("input") || !result.count("output")) {
        std::cout << options.help() << std::endl;
        return 1;
    }

    std::string modelPath = result["model"].as<std::string>();
    std::string imagePath = result["input"].as<std::string>();
    std::string outputPath = result["output"].as<std::string>();
    int gpuId = result["gpu"].as<int>();
    bool useGpu = gpuId >= 0;

    // ==========================================================
    // Default model configuration
    std::vector<float> mean = {0.0,0.0,0.0}; // Default mean
    std::vector<float> scale = {1.0,1.0,1.0}; // Default scale

    // Default map the class indices to colors
    std::vector<ClassInfo> classes = {
        {"Building", {{220, 20, 60}}},   // Class 0: Building
        {"Road", {{128, 64, 128}}},      // Class 1: Road
        {"Vegetation", {{60, 142, 35}}}, // Class 2: Vegetation
        {"Vehicle", {{180, 180, 0}}},    // Class 3: Vehicle
        {"Human", {{20, 140, 160}}},     // Class 4: Human
        {"Water", {{20, 20, 150}}}       // Class 5: Water
    };

    // ==========================================================
    // override default values with config file if available
    std::string configPath = modelPath.substr(0, modelPath.find_last_of('.')) + ".json";

    if (std::filesystem::exists(configPath)) {
        std::cout << "Loading config: " << configPath << std::endl;
        ModelConfig config = load_config(configPath);
        mean = config.mean;
        scale = config.std;
        classes = config.classes;
    }

    
    // ==========================================================
    // Load the model
    std::cout << "Loading model from: " << modelPath << std::endl;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    auto model_result = NeuralNetFactory::create(modelPath, useGpu, gpuId);

    if (!model_result)
    {
        std::cerr << "Failed to load model: " << model_result.error() << std::endl;
        return 1;
    }

    auto model = model_result.value();
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    //===========================================================
    // Print model information
    std::cout << "Model loaded successfully in: " << duration.count() << " ms" << std::endl;
    std::cout << "  Input shape:  " << shapeToString(model->inputShape()) << std::endl;
    std::cout << "  Output shape: " << shapeToString(model->outputShape()) << std::endl;
    std::cout << "  GPU ID:       " << model->gpuId() << std::endl;
    std::cout << "  Using GPU:    " << (model->gpuId() >= 0 ? "Yes" : "No") << std::endl;

    // ==========================================================
    // Load the input image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty())
    {
        std::cerr << "Failed to load image: " << imagePath << std::endl;
        return 1;
    }

    // Print image information
    std::cout << "Image loaded successfully: " << imagePath << std::endl;
    std::cout << "  Image size:   " << image.size() << std::endl;
    std::cout << "  Image type:   " << image.type() << std::endl;

    //===========================================================
    // Perform inference
    std::cout << "Running inference..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    auto tensor_result = Tensor::fromCvMat(image, model->inputShape(), 1.0f, mean, scale)
                             .and_then(Util::bind_inference(model))
                             .and_then(Util::bind_squeeze());

    if (!tensor_result)
    {
        std::cerr << "Inference failed: " << tensor_result.error() << std::endl;
        return 1;
    }
    auto tensor = std::move(tensor_result.value());

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Inference completed in: " << duration.count() << " ms" << std::endl;

    // Print the output tensor information
    std::cout << "Output tensor: " << tensor.toString() << std::endl;

    // ==========================================================
    // Colorize the output tensor based on class indices
    auto colorized_image_result = tensor.map([classes](int64_t value) -> std::array<uint8_t, 3>
                                             {
                                                 return classes[value].color; // Map class index to color
                                             },
                                             0)
                                      .and_then(Util::bind_toCvMat());

    if (!colorized_image_result)
    {
        std::cerr << "Failed to colorize tensor: " << colorized_image_result.error() << std::endl;
        return 1;
    }

    cv::Mat output_mat = std::move(colorized_image_result.value());
    cv::cvtColor(output_mat, output_mat, cv::COLOR_RGB2BGR); // Convert back to BGR for OpenCV
    cv::imwrite(outputPath + "/segmentation.png", output_mat);
    std::cout << "Output image saved to: " << outputPath + "/segmentation.png" << std::endl;

    // ==========================================================
    // Overlay the segmentation result on the original image
    if (image.size() != output_mat.size())
    {
        std::cerr << "Warning: Output image size does not match input image size. Resizing output image." << std::endl;
        cv::resize(output_mat, output_mat, image.size(), 0, 0, cv::INTER_NEAREST);
    }

    cv::Mat overlay;
    cv::addWeighted(image, 0.5, output_mat, 0.5, 0.0, overlay);
    cv::imwrite(outputPath + "/overlay.png", overlay);
    std::cout << "Overlay image saved to: " << outputPath + "/overlay.png" << std::endl;

    return 0;
}