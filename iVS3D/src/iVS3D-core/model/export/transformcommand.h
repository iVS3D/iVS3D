#ifndef TRANSFORMCOMMAND_H
#define TRANSFORMCOMMAND_H

#include <QDir>
#include <QRect>
#include <QString>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <optional>
#include <vector>

#include "imageprocessor.h"
#include "itransform.h"
#include "modelinputpictures.h"

class TransformCommand : public ImageCommand {
    ITransform *transformplugin;
    QString folder;
    Resolution working_resolution;
    ROI roi;
    cv::Size export_size;
    bool initialized;

   public:
    TransformCommand(ITransform *transformplugin, Resolution working_resolution,
                     Resolution export_resolution, ROI roi, QString folder);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif  // TRANSFORMCOMMAND_H
