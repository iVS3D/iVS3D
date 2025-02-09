#ifndef CROPCOMMAND_H
#define CROPCOMMAND_H

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
#include "metadatareader.h"
#include "imageprocessor.h"

class CropCommand : public ImageCommand {
    cv::Rect roi;

public:
    CropCommand(QRect roi);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //CROPCOMMAND_H
