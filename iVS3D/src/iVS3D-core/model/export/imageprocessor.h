#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QString>
#include <QDir>
#include <QRect>
#include <QDebug>

#include <vector>
#include <memory>
#include <optional>

#include "reader.h"

/**
 * @brief The ImageContext class provides contetx information for executing ImageCommands. This data can be modified by each command and is passed to the next command.
 */
class ImageContext {
public:
    cv::Mat image;
    uint index;
    QString filename;
};

/**
 * @brief The ImageCommand class is an abstract interface for commands regarding image operations such as resizing, exporting or adding metadata.
 */
class ImageCommand {
public:
    virtual std::optional<QString> execute(ImageContext &ctx) = 0;
    virtual ~ImageCommand() = default;
};

class ImageProcessor {
    std::vector<std::unique_ptr<ImageCommand>> commands;

public:
    void addCommand(std::unique_ptr<ImageCommand> cmd);
    bool process(ImageContext &context);
};

#endif //IMAGEPROCESSOR_H
