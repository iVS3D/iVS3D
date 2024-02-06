TEMPLATE = subdirs

SUBDIRS += \
    iVS3D-pluginInterface \
    #iVS3D-semanticSegmentationPlugin \
    iVS3D-blurPlugin \
    iVS3D-core \
    iVS3D-nthFramePlugin \
#    iVS3D-stationaryCameraPlugin \
    iVS3D-smoothCameraMovementPlugin \
    iVS3D-geoDistancePlugin \

iVS3D-core.depends = iVS3D-pluginInterface
#iVS3D-semanticSegmentationPlugin.depends = iVS3D-pluginInterface
iVS3D-blurPlugin.depends = iVS3D-pluginInterface
iVS3D-nthFramePlugin.depends = iVS3D-pluginInterface
#iVS3D-stationaryCameraPlugin.depends = iVS3D-pluginInterface
iVS3D-smoothCameraMovementPlugin.depends = iVS3D-pluginInterface
iVS3D-geoDistancePlugin.depends = iVS3D-pluginInterface
