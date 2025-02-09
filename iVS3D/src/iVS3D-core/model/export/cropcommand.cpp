#include "cropcommand.h"

CropCommand::CropCommand(QRect roi_) : roi(roi_.topLeft().x(), roi_.topRight().y(), roi_.width(), roi_.height()) { }

std::optional<QString> CropCommand::execute(ImageContext &ctx)
{
    ctx.image = ctx.image(roi);
    return std::nullopt;
}
