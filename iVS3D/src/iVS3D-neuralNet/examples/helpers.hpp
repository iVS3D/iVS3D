#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>

struct ClassInfo {
    std::string name;
    std::array<uint8_t, 3> color; // RGB color
};

struct ModelConfig
{
    std::vector<float> mean;
    std::vector<float> std;
    std::vector<ClassInfo> classes;
};

ModelConfig load_config(const std::string &config_path)
{
    cv::FileStorage fs(config_path, cv::FileStorage::READ | cv::FileStorage::FORMAT_JSON);
    if (!fs.isOpened()) {
        throw std::runtime_error("Could not open config file: " + config_path);
    }

    ModelConfig config;

    cv::FileNode mean_node = fs["mean"];
    for (const auto& val : mean_node) {
        config.mean.push_back((float)val);
    }

    cv::FileNode std_node = fs["std"];
    for (const auto& val : std_node) {
        config.std.push_back((float)val);
    }

    cv::FileNode classes_node = fs["classes"];
    for (const auto& class_node : classes_node) {
        ClassInfo class_info;
        class_node["name"] >> class_info.name;
        //class_node["color"] >> class_info.color;
        cv::FileNode color_node = class_node["color"];
        for (int i = 0; i < 3; ++i) {
            if (color_node[i].isInt()) {
                color_node[i] >> class_info.color[i];
            } else {
                throw std::runtime_error("Color values must be integers in range [0, 255]");
            }
        }
        config.classes.push_back(class_info);
    }

    return config;
}