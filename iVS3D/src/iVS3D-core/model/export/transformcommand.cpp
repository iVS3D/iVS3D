#include "transformcommand.h"
#include <iostream>

TransformCommand::TransformCommand(ITransform *transformplugin_,
                                   Resolution working_resolution_,
                                   Resolution export_resolution_, 
                                   ROI roi_,
                                   QString folder_)
    : transformplugin(transformplugin_),
      folder(folder_),
      working_resolution(working_resolution_),
      roi(roi_),
      initialized(false) {
        // compute cropped export resolution
        if(!roi.isDefault()){
            auto rect = roi.cropAsCvRect(export_resolution_);
            export_size = rect.size();
        } else {
            export_size = export_resolution_.toCvSize();
        }

      }

std::optional<QString> TransformCommand::execute(ImageContext &ctx) {
    // transform the image
    // we always pass the original image and the transform plugin should handle
    // resizing and cropping to the working resolution and roi
    auto result =
        transformplugin->transform(0, ctx.originalImage, working_resolution, roi);

    if (!result) {
        return "ERROR: " + result.error().message;
    }

    auto mask = result.value();

    if (mask.empty()) {
        return "ERROR: transform returned an empty image";
    }

    // resize the mask to the cropped export resolution, do not interpolate!
    if (mask.cols != export_size.width || mask.rows != export_size.height) {
        cv::resize(mask, mask, export_size, 0, 0, cv::INTER_NEAREST);
    }

    QString base_path = folder + "/masks";
    QString imagename = ctx.filename.split("/").last();
    // masks are always exported as png
    imagename.replace(QRegularExpression("\\.\\w+$"), ".png");

    if (!initialized) {
        QString destination = QDir::cleanPath(base_path);
        if (!QDir().exists(destination) && !QDir().mkpath(destination)) {
            return "ERROR: failed to create output folder for plugin: " +
                   destination;
        }
        initialized = true;
    }

    QString destination = QDir::cleanPath(base_path + "/" + imagename);
    // write image on disk
    if (!cv::imwrite(destination.toStdString(), mask)) {
        return "ERROR: failed to write image: " + destination;
    }

    return std::nullopt;
}
