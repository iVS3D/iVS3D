[Documentation](../README.md) / Third Party Dependencies

# Third Party Dependencies

For including 3rd party dependencies such as OpenCV and CUDA we rely on using _3rdparty.pri_ in combination with _.pri_ files for each dependency.

## OpenCV
You can build OpenCV from source following [these](https://docs.opencv.org/4.5.0/d3/d52/tutorial_windows_install.html) instructions. Make sure to build with OpenCV contrib. For windows set cmake flag `BUILD_opencv_world`.

Other [usefull cmake flags](https://docs.opencv.org/4.5.0/db/d05/tutorial_config_reference.html) are:
- `WITH_CUDA` to build with CUDA support
- `OPENCV_EXTRA_MODULES_PATH` to add opencv contrib modules
- `CMAKE_INSTALL_PREFIX` set install location


After OpenCV has been installed create a _opencv450.pri_ file in the installation folder. And add the following content:
<details><summary>Windows</summary>
<p>

```sh
message(USING opencv-4.5.0-msvc2015-cuda)

OPENCV_VERSION = "450"
OPENCV_PATH = $$PWD
OPENCV_INC_PATH = $$OPENCV_PATH/include
OPENCV_LIB_PATH = $$OPENCV_PATH/x64/vc15/lib

# Debug messages
!build_pass:message(OPENCV_INC_PATH was set to $${OPENCV_INC_PATH})
!build_pass:message(OPENCV_LIB_PATH was set to $${OPENCV_LIB_PATH})

INCLUDEPATH += $${OPENCV_INC_PATH}

win32 {
    DEFINES += _CRT_SECURE_NO_WARNINGS

    OPENCV_LIBS += opencv_worldVERSIONLIB

    # Add version number
    OPENCV_LIBS = $$replace(OPENCV_LIBS,VERSION,$${OPENCV_VERSION})

    QMAKE_LIBDIR += $${OPENCV_LIB_PATH}

    build_pass:CONFIG(debug, debug|release) {
        OPENCV_LIBS = $$replace(OPENCV_LIBS,LIB,d.lib)
    } else {
        OPENCV_LIBS = $$replace(OPENCV_LIBS,LIB,.lib)
    }

    LIBS += $${OPENCV_LIBS}
}
```

You might have to adjust the `OPENCV_INC_PATH` and `OPENCV_LIB_PATH` to point to the correct folders.
</p>
</details>

<details><summary>Linux</summary>
<p>

```sh
message(USING opencv_450_cuda)

OCV4_PATH = $$PWD
INCLUDEPATH += $$OCV4_PATH/include
DEPENDPATH += $$OCV4_PATH/include
INCLUDEPATH += $$OCV4_PATH/include/opencv4
DEPENDPATH += $$OCV4_PATH/include/opencv4

# supress warings from included libraries
QMAKE_CXXFLAGS += -isystem $$OCV4_PATH/include
QMAKE_CXXFLAGS += -isystem $$OCV4_PATH/include/opencv4

unix {
    LIBS += -L$$OCV4_PATH/lib
    OCV4_LIBS += -lopencv_calib3d
    OCV4_LIBS += -lopencv_core
    OCV4_LIBS += -lopencv_dnn
    OCV4_LIBS += -lopencv_features2d
    OCV4_LIBS += -lopencv_flann
    OCV4_LIBS += -lopencv_gapi
    OCV4_LIBS += -lopencv_highgui
    OCV4_LIBS += -lopencv_imgcodecs
    OCV4_LIBS += -lopencv_imgproc
    OCV4_LIBS += -lopencv_ml
    OCV4_LIBS += -lopencv_objdetect
    OCV4_LIBS += -lopencv_photo
    OCV4_LIBS += -lopencv_stitching
    OCV4_LIBS += -lopencv_video
    OCV4_LIBS += -lopencv_videoio

    !CONFIG(MANUAL_OCV_LIB_LINKING) {
        LIBS += $$OCV4_LIBS
    }
}

windows {
    error(THIS IS NO WINDOWS BUILD)
}
```
You might have to add more modules to the `OCV4_LIBS` if you have built them.
</p>
</details>

This _opencv450.pri_ can be included when building iVS3D and provides all necessary information about OpenCV for the compiler and linker.

## CUDA
[NVIDIA CUDA Toolkit API] can be installed using the official installer. In this case make sure it is added to your `PATH` or `LD_LIBRARY_PATH`.

Or built from source as well. In this case you need to create a _cuda.pri_ file with content like this:

```sh
CUDA_VERS_STR = 10.0.130
GCC_VERS_STR = 7.3

CUDA_ROOT_DIR = $$PWD/cuda_10.0.130_linux
CUDA_SDK_DIR = $$PWD/cuda_10.0.130_linux/samples

# add directories and libraries to variables
unix {	
	message("Using cuda "$${CUDA_VERS_STR}", GCC "$${GCC_VERS_STR})

  INCLUDEPATH += $${CUDA_ROOT_DIR}/include/ \
                $${CUDA_SDK_DIR}/common/inc/
	DEPENDPATH += $${CUDA_ROOT_DIR}/include/
                $${CUDA_SDK_DIR}/common/inc/

	LIBS += -L$${CUDA_ROOT_DIR}/lib64  

	LIBS += -laccinj64
	LIBS += -lcublas
	LIBS += -lcudadevrt
	LIBS += -lcudart
	LIBS += -lcudnn
	LIBS += -lcufft
	LIBS += -lcufftw
	LIBS += -lcuinj64
	LIBS += -lculibos
	LIBS += -lcurand
	LIBS += -lcusolver
	LIBS += -lcusparse
	LIBS += -lnppc
	LIBS += -lnppial
	LIBS += -lnppicc
	LIBS += -lnppicom
	LIBS += -lnppidei
	LIBS += -lnppif
	LIBS += -lnppig
	LIBS += -lnppim
	LIBS += -lnppist
	LIBS += -lnppisu
	LIBS += -lnppitc
	LIBS += -lnpps
	LIBS += -lnvblas
	LIBS += -lnvgraph
	LIBS += -lnvjpeg
	LIBS += -lnvrtc-builtins
	LIBS += -lnvrtc
	LIBS += -lnvToolsExt

}
```

[NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>