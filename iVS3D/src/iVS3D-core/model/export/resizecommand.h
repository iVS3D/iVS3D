#ifndef RESIZECOMMAND_H
#define RESIZECOMMAND_H

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QString>
#include <QDir>
#include <QRect>

#include <vector>
#include <memory>
#include <optional>

#include "modelinputpictures.h"
#include "imageprocessor.h"

class ResizeCommand : public ImageCommand {
    cv::Size resolution;

public:
    ResizeCommand(QPoint resolution);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //RESIZECOMMAND_H
