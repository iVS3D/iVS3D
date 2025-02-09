#include "transformcommand.h"

TransformCommand::TransformCommand(ITransform *transformplugin_, QString folder_) : transformplugin(transformplugin_), folder(folder_), initialized(false) { }

std::optional<QString> TransformCommand::execute(ImageContext &ctx)
{
    ImageList transformed_imgs = transformplugin->transform(0, ctx.image);
    QStringList transformed_names = transformplugin->getOutputNames();

    if(transformed_imgs.length() != transformed_names.length()) {
        return "ERROR: Number of transformed images (" +
                QString::number(transformed_imgs.length()) +
                ") does not match expectaion (" +
                QString::number(transformed_names.length()) +
                ") for plugin " + transformplugin->getName();
    }

    QString base_path = folder + "/" + transformplugin->getName();
    QString imagename = ctx.filename.split("/").last();

    if (!initialized) {
        for(auto name : transformed_names) {
            QString destination = QDir::cleanPath(base_path+ "/" + name);
            if(!QDir().exists(destination) && !QDir().mkpath(destination)) {
                return "ERROR: failed to create output folder for plugin: " + destination;
            }
        }
        initialized = true;
    }

    for (int i = 0; i < transformed_imgs.size(); i++) {
        QString destination = QDir::cleanPath(base_path + "/" + transformed_names[i] + "/" + imagename);
        //write image on disk
        if (!cv::imwrite(destination.toStdString(), transformed_imgs[i], {cv::IMWRITE_JPEG_QUALITY, 100})) {
            return "ERROR: failed to write image: " + destination;
        }
    }

    return std::nullopt;
}
