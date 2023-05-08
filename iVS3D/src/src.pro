TEMPLATE = subdirs

SUBDIRS += \
    iVS3D-pluginInterface \
    #iVS3D-semanticSegmentationPlugin \
    iVS3D-blurPlugin \
    iVS3D-core \
    iVS3D-nthFramePlugin \
    #iVS3D-cameraMovementPlugin \
    iVS3D-stationaryCameraPlugin \

iVS3D-core.depends = iVS3D-pluginInterface
#iVS3D-semanticSegmentationPlugin.depends = iVS3D-pluginInterface
iVS3D-blurPlugin.depends = iVS3D-pluginInterface
iVS3D-nthFramePlugin.depends = iVS3D-pluginInterface
#iVS3D-cameraMovementPlugin.depends = iVS3D-pluginInterface
iVS3D-stationaryCameraPlugin.depends = iVS3D-pluginInterface
SUBDIRS += iVS3D-RandomPickerPlugin 
iVS3D-RandomPickerPlugin.depends = iVS3D-pluginInterface 
