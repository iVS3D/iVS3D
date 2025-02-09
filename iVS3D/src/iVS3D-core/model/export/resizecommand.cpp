#include "resizecommand.h"

ResizeCommand::ResizeCommand(QPoint resolution_) : resolution(resolution_.x(),resolution_.y()) { }

std::optional<QString> ResizeCommand::execute(ImageContext &ctx)
{
    cv::Mat resized_img;
    cv::resize(ctx.image, resized_img, resolution, 0, 0, cv::INTER_AREA);
    ctx.image = resized_img;
    return std::nullopt;
}
