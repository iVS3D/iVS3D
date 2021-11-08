TEMPLATE = subdirs

SUBDIRS += \
    iVS3D-pluginInterface \
    iVS3D-semanticSegmentationPlugin \
    iVS3D-blurPlugin \
    iVS3D-core \
    iVS3D-nthFramePlugin \
    iVS3D-cameraMovementPlugin

iVSR3D-core.depends = iVS3D-pluginInterface
iVSR3D-semanticSegmentationPlugin.depends = iVS3D-pluginInterface
iVS3D-blurPlugin.depends = iVS3D-core
iVS3D-nthFramePlugin.depends = iVS3D-core
iVS3D-cameraMovementPlugin.depends = iVS3D-core