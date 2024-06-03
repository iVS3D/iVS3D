set(TARGET_NAME StationaryCameraMovement)

# set include directories
include_directories(SYSTEM
    ${Qt${QT_VERSION_MAJOR}Core_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Gui_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-pluginInterface"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/reader"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/metaData"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/model/log"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/plugin"
)

# video and dnn from opencv is required for farneback
find_package(OpenCV 4.7 REQUIRED COMPONENTS
    video
    dnn
)

# CUDA (if applicable)
if(With_CUDA)

    # cudaoptflow from opencv is required
    find_package(OpenCV 4.7 REQUIRED COMPONENTS
        cudaoptflow
    )

    include_directories(SYSTEM
        ${CUDA_INCLUDE_DIRS}
        ${CUDNN_INCLUDE_DIR}
    )

    set(CUDA_HEADER_FILES
        "${CMAKE_CURRENT_LIST_DIR}/flowcalculatorcuda.h"
        "${CMAKE_CURRENT_LIST_DIR}/imagegatherercuda.h"
    )

    set(CUDA_SRC_FILES
        "${CMAKE_CURRENT_LIST_DIR}/flowcalculatorcuda.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/imagegatherercuda.cpp"
    )
endif()

# set header files
set(HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/factory.h"
    "${CMAKE_CURRENT_LIST_DIR}/flowcalculator.h"
    "${CMAKE_CURRENT_LIST_DIR}/flowcalculatorcpu.h"
    "${CMAKE_CURRENT_LIST_DIR}/imagegatherer.h"
    "${CMAKE_CURRENT_LIST_DIR}/imagegatherercpu.h"
    "${CMAKE_CURRENT_LIST_DIR}/keyframeselector.h"
    "${CMAKE_CURRENT_LIST_DIR}/optflowcontroller.h"
    "${CMAKE_CURRENT_LIST_DIR}/../iVS3D-core/cvmat_qmetadata.h"
    ${CUDA_HEADER_FILES}
)

# set source files
set(SRC_FILES
    "${CMAKE_CURRENT_LIST_DIR}/factory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/flowcalculator.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/flowcalculatorcpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/imagegatherer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/imagegatherercpu.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/keyframeselector.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/optflowcontroller.cpp"
    ${CUDA_SRC_FILES}
)

# set compiled translation files (.qm)
set(QM_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/stationary_de.qm"
    "${CMAKE_CURRENT_LIST_DIR}/translations/stationary_en.qm"
)

# set compiled translation files (.ts)
set(TS_FILES
    "${CMAKE_CURRENT_LIST_DIR}/translations/stationary_de.ts"
    "${CMAKE_CURRENT_LIST_DIR}/translations/stationary_en.ts"
)

# custom command to create translations (.qm) files using lrelease
add_custom_command(
    OUTPUT ${QM_FILES}
    COMMAND lrelease ${TS_FILES}
    DEPENDS ${TS_FILES}
)

# add ressources
qt5_add_resources(QRC_SRC_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/resources.qrc"
)

# define library
add_library(${TARGET_NAME} SHARED
    ${HEADER_FILES}
    ${SRC_FILES}
    ${QRC_SRC_FILES}
    ${QM_FILES}
    ${TS_FILES}
)
target_link_libraries(${TARGET_NAME}
    iVS3D-pluginInterface
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Positioning
    ${OpenCV_LIBRARIES} 
    ${CUDA_LIBRARIES}
    ${CUDNN_LIBRARIES}
)

# add defines
target_compile_definitions(${TARGET_NAME} PRIVATE 
    ${COMMON_COMPILE_DEFINITIONS}
)

# set rpath
set_target_properties(${TARGET_NAME} PROPERTIES
                      INSTALL_RPATH "$ORIGIN"
)

# add dependency to iVS3D_pluginInterface
add_dependencies(${TARGET_NAME}
    iVS3D-pluginInterface
    iVS3D-core
)

# install
install(TARGETS 
    ${TARGET_NAME} 
    DESTINATION bin/plugins
)

if(Update_Translations)
    # if Update_Translations flag is set to ON, we want to override existing .ts files with new Strings from the src files.
    # this is done using Qt's tool "lupdate", which searches all src files (.cpp and .h) in the given folder (in this case
    # ${CMAKE_CURRENT_LIST_DIR}) and outputs the updated .ts files
    # IMPORTANT: We want this command to execute, even if the ts files already exist, to make sure they are updated! This is
    # achieved by adding an OUTPUT (placeholder.out) which is never created and thus forces the command to run on every build.
    add_custom_command(
        OUTPUT ${TS_FILES} placeholder.out
        COMMAND lupdate ${CMAKE_CURRENT_LIST_DIR} -ts ${TS_FILES} -no-obsolete
        DEPENDS ${HEADER_FILES} ${SRC_FILES}
    )
endif()
