/**
 * @file inference_detection.cpp
 * @brief Example for performing object detection inference using a neural network model.
 *
 * @details This example demonstrates how to load an object detection model, perform inference on an input image,
 * and save the result with bounding boxes drawn on the image.
 *
 * @author Dominik Wüst
 * @date November 2025
 */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "NeuralNetFactory.h"
#include "NeuralUtil.h"
#include "cxxopts.hpp"

struct Detection {
    float x, y, width, height, confidence;
    int classId;
};

// Helper function to draw bounding boxes on the image
void drawBoundingBoxes(cv::Mat& image, const std::vector<Detection>& detections) {
    for (const auto& detection : detections) {
        // Generate a light color based on the classId using HSV
        int hue = (detection.classId * 45) % 360; // Randomize hue based on classId
        int saturation = 200; // High saturation for vibrant colors
        int value = 255; // High value for bright colors

        // Convert HSV to BGR
        cv::Mat hsv(1, 1, CV_8UC3, cv::Scalar(hue / 2, saturation, value)); // OpenCV uses hue in [0, 180]
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
        cv::Vec3b bgrColor = bgr.at<cv::Vec3b>(0, 0);
        cv::Scalar color(bgrColor[0], bgrColor[1], bgrColor[2]);

        // Draw rectangle
        cv::Rect rect(detection.x, detection.y, detection.width, detection.height);
        cv::rectangle(image, rect, color, 2);

        // Draw label
        std::string label = "Class " + std::to_string(detection.classId) + " (" + std::to_string(detection.confidence * 100) + "%)";
        int baseLine;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        cv::rectangle(image, cv::Point(rect.x, rect.y - labelSize.height - 5),
                      cv::Point(rect.x + labelSize.width, rect.y), color, cv::FILLED);
        cv::putText(image, label, cv::Point(rect.x, rect.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }
}

int main(int argc, char** argv) {
    using namespace NN;

    // =========================================================
    // Command line argument parsing using cxxopts
    cxxopts::Options options(argv[0], "Object detection inference example");
    options.add_options()
        ("m,model", "Path to model", cxxopts::value<std::string>())
        ("i,input", "Path to input image", cxxopts::value<std::string>())
        ("o,output", "Output path", cxxopts::value<std::string>())
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
    // Load the model
    std::cout << "Loading model from: " << modelPath << std::endl;
    auto model_result = NeuralNetFactory::create(modelPath, useGpu, gpuId);

    if (!model_result) {
        std::cerr << "Failed to load model: " << model_result.error() << std::endl;
        return 1;
    }

    auto model = model_result.value();
    std::cout << "Model loaded successfully." << std::endl;

    // ==========================================================
    // Load the input image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Failed to load image: " << imagePath << std::endl;
        return 1;
    }

    // ==========================================================
    // Perform inference
    std::cout << "Running inference..." << std::endl;
    auto tensor_result = Tensor::fromCvMat(image, model->inputShape(), 1.0f/255.0f)
                             .and_then(Util::bind_inference(model))
                             .and_then(Util::bind_selectOutput(0));

    if (!tensor_result) {
        std::cerr << "Inference failed: " << tensor_result.error() << std::endl;
        return 1;
    }

    auto tensor = std::move(tensor_result.value());
    std::cout << "Inference completed." << std::endl;

    // ==========================================================
    // Parse the output tensor
    std::vector<Detection> detections;
    
    auto expected_data = tensor.toVector<float>();
    if (!expected_data) {
        std::cerr << "Failed to convert output tensor to vector: " << expected_data.error() << std::endl;
        return 1;
    }
    const auto& data = expected_data.value();
    auto shape = tensor.shape();

    SHAPE_DEBUG_PRINT(model->inputShape());
    TENSOR_DEBUG_PRINT(tensor);

    cv::Size tensorSize(model->inputShape()[3], model->inputShape()[2]);
    float scaleX = static_cast<float>(image.cols) / tensorSize.width;
    float scaleY = static_cast<float>(image.rows) / tensorSize.height;

    // Assuming YOLO output format: [batch, num_detections, 6] -> [x, y, w, h, confidence, class_id]
    for (size_t i = 0; i < shape[1]; ++i) {
        float confidence = data[i * 6 + 4];
        if (confidence > 0.5f) { // Confidence threshold
            Detection detection;
            detection.x = data[i * 6 + 0] * scaleX;
            detection.y = data[i * 6 + 1] * scaleY;
            detection.width = (data[i * 6 + 2] - data[i * 6 + 0]) * scaleX;
            detection.height = (data[i * 6 + 3] - data[i * 6 + 1]) * scaleY;
            detection.confidence = confidence;
            detection.classId = static_cast<int>(data[i * 6 + 5]);
            detections.push_back(detection);
        }
    }

    std::cout << "Detections found: " << detections.size() << std::endl;
    std::cout << std::fixed << std::setprecision(2)
              << "i.e.: (" << detections[0].x << "," << detections[0].y << ") (" 
              << detections[0].width << "," << detections[0].height << ") " << detections[0].confidence 
              << "[" << detections[0].classId << "]" << std::endl;
    // ==========================================================
    // Draw bounding boxes
    drawBoundingBoxes(image, detections);

    // ==========================================================
    // Save the output image
    cv::imwrite(outputPath, image);
    std::cout << "Output image saved to: " << outputPath << std::endl;

    return 0;
}