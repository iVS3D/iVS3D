#ifndef TRANSFORMCOMMAND_H
#define TRANSFORMCOMMAND_H

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
#include "itransform.h"

class TransformCommand : public ImageCommand {
    ITransform *transformplugin;
    QString folder;
    bool initialized;

public:
    TransformCommand(ITransform *transformplugin, QString folder);
    std::optional<QString> execute(ImageContext &ctx) override;
};

#endif //TRANSFORMCOMMAND_H
