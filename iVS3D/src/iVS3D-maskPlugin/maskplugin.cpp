#include "maskplugin.h"

MaskResult MaskPlugin::generateMask(const MaskData& data) {
    return data.image.clone();
}

Visualization MaskPlugin::generatePreview(const PreviewData& data) {
    return Visualization();
}
