[Documentation](../README.md) / Deploy from source for windows

# Deploy from source for windows
To build the project from source on a windows machine you can use the terminal or download Qt Creator from the official Qt website. If you want to use the Qt Creator follow our instructions [here](build_qtcreator.md). To use the terminal follow the instructions in this document.

[Qt] 5.15.2 and [Visual Studio] 2019 or 2022 have to be installed. Make sure to select [MSVC] 2019 or later on both installations. Clone the iVS3D repository from our GitHub and download [OpenCV] 4.7.0. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Copy the _3rdparty.txt_ file, rename to 3rdparty.pri and set the paths to your OpenCV build there. The file should look something like this:

```sh
with_cuda{
    message(USING OPENCV WITH CUDA)

    # path to OpenCV 4.7.0 with CUDA
    include(C:\OpenCV\opencv-4.7.0-msvc2019-cuda12\opencv-4.7.0-msvc2019-cuda12.pri)
} else {
    message(USING OPENCV WITHOUT CUDA)

    #path to OpenCV 4.7.0 without CUDA
    include(C:\OpenCV\opencv-4.7.0-msvc2019\opencv-4.7.0-msvc2019.pri)
}
```
## Setup the deploy.bat script
Open the deploy.bat file we provide in the tools folder and set your paths in the variables-section.

### Configure [Visual Studio] as follows:
| variable     | supported values          | default   |
|--------------|---------------------------|-----------|
| VS_VERSION   | 2019, 2022                | 2022      |
| VS_EDITION   | Community, Enterprise     | Community |

In case you did not use the default install location you might need to change the `VSVARS_PATH`. The default value is 
```sh
C:\Program Files\Microsoft Visual Studio\%VS_VERSION%\%VS_EDITION%\VC\Auxiliary\Build
```
This location needs to contain the `vcvarsall.bat` script for initializing the [MSVC] environment.

### Configure [Qt] as follows:
| variable     | default value                 | Files                                                    |
|--------------|-------------------------------|----------------------------------------------------------|
| QT_PATH      | C:\Qt\5.15.2\msvc2019_64      | bin\lrelease-pro.exe, bin\qmake.exe, bin\windeployqt.exe |
| JOM_PATH     | C:\Qt\Tools\QtCreator\bin\jom | jom.exe                                                  |

### Configure [OpenCV] as follows:
| variable     | default value                                             | Files                |
|--------------|-----------------------------------------------------------|----------------------|
| OCV_BIN      | C:\OpenCV\opencv_4.7.0_msvc2019_win_x64\x64\vc16\bin      | opencv_world470.dll  |

### Configure [NVIDIA CUDA Toolkit API] as follows:
CUDA support is optional. If you dont want to use it, make sure `CUDA_VERSION` is not defined. Otherwise you can select a CUDA version which matches your OpenCV build like this:
```sh
set CUDA_VERSION=12.0
```
In case you did not use the default install location you might need to change `CUDA_BIN`. The default path is 
```sh
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%CUDA_VERSION%\bin
```

### Configure install location as follows:
Select the install directory for the finished build by changing `INSTALL_PATH`. The default path is inside the `Releases` directory in your iVS3D repository.

## Run the deploy.bat script
Save the changes and run it to deploy the application.

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [Visual Studio]: <https://visualstudio.microsoft.com/de>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
