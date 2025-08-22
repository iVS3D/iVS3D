#include "transformcommand.h"

TransformCommand::TransformCommand(ITransform *transformplugin_, QString folder_) : transformplugin(transformplugin_), folder(folder_), initialized(false) { }

std::optional<QString> TransformCommand::execute(ImageContext &ctx)
{
    auto result = transformplugin->transform(0, ctx.image, Resolution(ctx.image), ROI());

    if(!result) {
        return "ERROR: " + result.error().message;
    }

    auto mask = result.value();

    if (mask.empty()) {
        return "ERROR: transform returned an empty image";
    }

    QString base_path = folder + "/masks";
    QString imagename = ctx.filename.split("/").last();
    // masks are always exported as png
    imagename.replace(QRegularExpression("\\.\\w+$"), ".png");
    
    if (!initialized) {
        QString destination = QDir::cleanPath(base_path);
        if(!QDir().exists(destination) && !QDir().mkpath(destination)) {
            return "ERROR: failed to create output folder for plugin: " + destination;
        }
        initialized = true;
    }

    QString destination = QDir::cleanPath(base_path + "/" + imagename);
    //write image on disk
    if (!cv::imwrite(destination.toStdString(), mask)) {
        return "ERROR: failed to write image: " + destination;
    }

    return std::nullopt;
}
