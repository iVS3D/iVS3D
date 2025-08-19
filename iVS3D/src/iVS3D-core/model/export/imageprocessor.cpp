#include "imageprocessor.h"

void ImageProcessor::addCommand(std::unique_ptr<ImageCommand> cmd)
{
    commands.push_back(std::move(cmd));
}

std::optional<QString> ImageProcessor::process(ImageContext &context)
{
    for (auto& cmd : commands) {
        if(auto err = cmd->execute(context)) {
            qDebug() << *err;
            return *err;
        }
    }
    return std::nullopt;
}
