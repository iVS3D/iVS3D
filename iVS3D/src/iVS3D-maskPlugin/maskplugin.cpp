#include "maskplugin.h"

MaskResult MaskPlugin::generateMask(const MaskData& data) {
    return data.image.clone();
}

Visualization MaskPlugin::generatePreview(const PreviewData& data) {

    Visualization vis;
    
    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Mask Preview");
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::FullImage;
        view.style.showTitle = true;

        RectOverlay overlay;
        overlay.rectangle = QRectF(0.2,0.2,0.6,0.6); // normalized coordinates
        overlay.style.strokeColor = Qt::green;
        overlay.style.strokeWidth = 3;
        
        view.overlays.push_back(overlay);
    }

    {
        auto& view = vis.views.emplace_back();
        view.title = tr("Region of Interest");
        view.style.backgroundColor = Qt::transparent;
        view.style.viewport = ViewportType::RegionOfInterest;
        view.style.showTitle = true;

        RectOverlay overlay;
        overlay.rectangle = QRectF(0.01,0.01,0.98,0.98); // normalized coordinates
        overlay.style.strokeColor = Qt::red;
        overlay.style.strokeWidth = 3;
        overlay.style.fillColor = QColor(255,0,0,100); // semi-transparent red
        
        view.overlays.push_back(overlay);
    }

    return vis;
}
