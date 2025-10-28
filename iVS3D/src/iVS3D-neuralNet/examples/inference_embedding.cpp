/**
 * @file inference_embedding.cpp
 * @brief Example for performing image embedding using a neural network model like CosPlace or EigenPlaces.
 * 
 * @details This example demonstrates how to load an embedding model, perform inference on one or more input images,
 * and save the embedding results. It supports GPU inference if available.
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date June 2025
 */

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <memory>
#include <filesystem>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "NeuralNetFactory.h"
#include "NeuralUtil.h"

#include "cxxopts.hpp"

#include "helpers.hpp"

/**
 * @brief Main function for embedding inference example.
 * 
 * This example demonstrates how to load an embedding model,
 * perform inference on one or more input images, and save the embedding results.
 * It also supports GPU inference if available.
 * 
 * Usage:
 * ./inference_embedding --model <model_path> --input <image_path> [--gpu <gpu_id>]
 * 
 * The model needs to be an embedding model in onnx format.
 */
int main(int argc, char **argv)
{
    using namespace NN;

    // =========================================================
    // Command line argument parsing using cxxopts
    cxxopts::Options options(argv[0], "Embedding inference example");
    options.add_options()
        ("m,model", "Path to model", cxxopts::value<std::string>())
        ("i,input", "Path to input image or folder containing images", cxxopts::value<std::string>())
        ("o,output", "Output .yml file for storing embeddings", cxxopts::value<std::string>()->default_value("embeddings.yml"))
        ("gpu", "GPU id (-1 for CPU)", cxxopts::value<int>()->default_value("0"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help") || !result.count("model") || !result.count("input")) {
        std::cout << options.help() << std::endl;
        return 1;
    }

    std::string modelPath = result["model"].as<std::string>();
    std::string inputPath = result["input"].as<std::string>();
    std::string outputPath = result["output"].as<std::string>();
    int gpuId = result["gpu"].as<int>();

    // =========================================================
    // Load the neural network model
    auto model_result = NeuralNetFactory::create(modelPath, gpuId >= 0, gpuId);
    if (!model_result) {
        std::cerr << "Failed to load model from " << modelPath << std::endl;
        return 1;
    }
    auto model = model_result.value();

    std::cout << "Model loaded successfully: " << modelPath << std::endl;
    std::cout << "  Input shape:  " << shapeToString(model->inputShape()) << std::endl;
    std::cout << "  Output shape: " << shapeToString(model->outputShape()) << std::endl;
    std::cout << "  GPU ID:       " << model->gpuId() << std::endl;
    std::cout << "  Using GPU:    " << (model->gpuId() >= 0 ? "Yes" : "No") << std::endl;

    // =========================================================
    // Prepare input images
    std::cout << "Loading input images from: " << inputPath << std::endl;
    std::vector<cv::Mat> inputImages;
    std::vector<std::string> inputImageNames;
    if (std::filesystem::is_directory(inputPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(inputPath)) {
            if (entry.is_regular_file()) {
                cv::Mat image = cv::imread(entry.path().string());
                if (!image.empty()) {
                    inputImages.push_back(image);
                    inputImageNames.push_back(entry.path().filename().string());
                    std::cout << "Loaded image: " << entry.path().filename() << std::endl;
                }
            }
        }
    } else {
        cv::Mat image = cv::imread(inputPath);
        if (!image.empty()) {
            inputImages.push_back(image);
            std::cout << "Loaded single image: " << inputPath << std::endl;
        }
    }

    if (inputImages.empty()) {
        std::cerr << "No valid input images found." << std::endl;
        return 1;
    }

    if (model->inputShape().size() != 4) {
        std::cerr << "Model input shape is invalid. Expected exactly 4 dimensions for NCHW layout." << std::endl;
        return 1;
    }

    

    // =========================================================
    // Perform inference
    
    std::vector<std::vector<float>> embeddings;
    auto batchSize = model->inputShape()[0];
    if (batchSize == 1) {
        std::cout << "Performing single image inference..." << std::endl;
        // Single image inference
        for (const auto& image : inputImages) {
            auto embedding_result = Tensor::fromCvMat(image, model->inputShape(), 1.0f)
                                     .and_then(Util::bind_inference(model))
                                     .and_then(Util::bind_squeeze())
                                     .and_then(Util::bind_toVector<float>());

            if (!embedding_result) {
                std::cerr << "Inference failed for image: " << embedding_result.error() << std::endl;
                return 1;
            }
            embeddings.push_back(embedding_result.value());
        }
    } else {
        std::cout << "Performing batch inference with batch size: " << batchSize << std::endl;
        // Batch inference
        auto embedding_result = Tensor::fromCvMats(inputImages, model->inputShape(), 1.0f)
                                .and_then(Util::bind_inference(model))
                                .and_then(Util::bind_squeeze())
                                .and_then(Util::bind_toVector<float>());

        if (!embedding_result) {
            std::cerr << "Batch inference failed: " << embedding_result.error() << std::endl;
            return 1;
        }
        // we need to split the result into individual embeddings
        auto embedding_size = embedding_result.value().size() / inputImages.size();
        for (size_t i = 0; i < inputImages.size(); ++i) {
            std::vector<float> embedding(embedding_result.value().begin() + i * embedding_size,
                                         embedding_result.value().begin() + (i + 1) * embedding_size);
            embeddings.push_back(embedding);
        }
    }
    
    if (embeddings.size() != inputImages.size()) {
        std::cerr << "Number of embeddings does not match number of input images." << std::endl;
        return 1;
    }

    // =========================================================
    // Save embeddings as json to output file using opencv
    cv::FileStorage fs(outputPath, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        return 1;
    }


    fs << "data" << "[";
    for (size_t i = 0; i < inputImageNames.size(); ++i) {
        fs << "{"
           << "name" << inputImageNames[i]
           << "embedding" << embeddings[i]
           << "}";
    }
    fs << "]";
    fs.release();
    std::cout << "Embeddings saved to: " << outputPath << std::endl;

    return 0;
}