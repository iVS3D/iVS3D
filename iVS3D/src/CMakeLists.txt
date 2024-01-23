# add subdirectories
add_subdirectory(iVS3D-core)
add_subdirectory(iVS3D-pluginInterface)

if(Build_Plugins)
    add_subdirectory(iVS3D-blurPlugin)
    #add_subdirectory(iVS3D-cameraMovementPlugin)
    add_subdirectory(iVS3D-nthFramePlugin)
    #add_subdirectory(iVS3D-semanticSegmentationPlugin)
    add_subdirectory(iVS3D-stationaryCameraPlugin)
    add_subdirectory(iVS3D-geoDistancePlugin)
endif()
