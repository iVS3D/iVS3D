# add subdirectories
add_subdirectory(iVS3D-core)
add_subdirectory(iVS3D-pluginInterface)

if(Build_Plugins)
    add_subdirectory(iVS3D-blurPlugin)
    add_subdirectory(iVS3D-nthFramePlugin)
    add_subdirectory(iVS3D-semanticSegmentationPlugin)
    add_subdirectory(iVS3D-stationaryCameraPlugin)
    add_subdirectory(iVS3D-smoothCameraMovementPlugin)
    add_subdirectory(iVS3D-geoDistancePlugin)
    add_subdirectory(iVS3D-geoMapPlugin)
    add_subdirectory(iVS3D-visualSimilarityPlugin)
endif()

if(Install_Models)
    add_subdirectory(iVS3D-models)
endif()

