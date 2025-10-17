# Deploy from source for windows

## Prerequisites
[Qt] 5.15.2 and [Visual Studio] 2019 or 2022 have to be installed. Make sure to select [MSVC] 2019 or later on both installations. Please add the Qt binary folder to your PATH variable, this is necessary for compiling translations and ui files using tools provided in the Qt installation, e.g.:
```powershell
$env:PATH += ";C:\Qt\MSVC2019\bin"
```

Download [OpenCV] 4.11.0 with contrib modules or build it from source. Your OpenCV build should contain a file `OpenCVConfig.cmake`. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] 12.0 as well.

For the Neural Network based plugins, you need to download [Onnxruntime] 1.18.0 or later GPU (make sure to select the version according to your CUDA version) as well.

For video decoding, download the latest shared build of [Ffmpeg].

Finally, install cmake 3.22 or later, git and git-lfs.

## Clone iVS3D

Clone iVS3D recursively to include the neural network models for our plugins.
```powershell
git clone --recursive https://github.com/iVS3D/iVS3D.git
Set-Location -Path iVS3D
```

## Configure, build and install with cmake presets

We use cmake presets to simplify the configuration process. In the project root, create a new `CMakeUserPresets.json` file to override the default presets:

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 22,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "usr-windows-release",
            "displayName": "User Specific Windows Release",
      	    "description": "Windows build in Release mode",
            "inherits": "windows-release",
            "cacheVariables": {
                "OpenCV_DIR": "C:\\3rdParty\\opencv_4.11.0_contrib_cuda12.8_msvc2022_windows11_x64",
                "Onnxruntime_ROOT_DIR": "C:\\3rdParty\\onnxruntime-win-x64-gpu-1.22.0",
                "Build_Tests": "OFF",
                "Update_Translations": "OFF",
                "Install_Models": "ON",
                "Ffmpeg_ROOT_DIR": "C:\\3rdParty\\ffmpeg-master-latest-win64-gpl-shared"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "usr-windows-release",
            "configurePreset": "usr-windows-release",
            "displayName": "User Specific Windows Release",
            "configuration": "Release",
            "jobs": 12
        }
    ]
}
```
- OpenCV_DIR: Path to the folder containing OpenCVConfig.cmake
- Onnxruntime_ROOT_DIR: Path to the directory containing `lib` and `include` folders of onnxruntime
- Ffmpeg_ROOT_DIR: Path to the directory containing `bin`, `lib` and `include` folders of ffmpeg


```powershell
New-Item -Path "build" --ItemType Directory
Set-Location -Path build
cmake \
  -DOpenCV_DIR="<path to opencv folder containing OpenCVConfig.cmake>" \
  -DWith_CUDA="ON" \
  ..
```
With this in place, you can configure your build using the preset. From the iVS3D root folder run:
```powershell
cmake --preset usr-windows-release --config Release
``` 

<details>
<summary>Expected CMake output</summary>

```plaintext
[cmake] -- ======================================
[cmake] --   Project Configuration Summary
[cmake] -- ======================================
[cmake] -- Build type:         Release
[cmake] -- Install prefix:     C:/iVS3D/build/windows-release
[cmake] -- 
[cmake] -- 3rd Party Dependencies:
[cmake] --   Qt                        YES  (5.15.2)
[cmake] --   OpenCV                    YES  (4.11.0)
[cmake] --   Onnxruntime               YES  (1.22.0)
[cmake] --   Ffmpeg                    YES 
[cmake] --     - avformat                   (62)
[cmake] --     - avcodec                    (62)
[cmake] --     - avutil                     (60)
[cmake] --     - swscale                    (9)
[cmake] --   CUDA                      YES  (12.8)
[cmake] -- 
[cmake] -- Components:
[cmake] --   iVS3D-core                YES  (2.0.0.12)
[cmake] --   NeuralNet                 YES 
[cmake] -- 
[cmake] -- Plugins:
[cmake] --   Blur                      YES 
[cmake] --   0_NthFrame                YES 
[cmake] --   SemanticSegmentation      YES 
[cmake] --   StationaryCameraMovement  YES 
[cmake] --   SmoothCameraMovement      YES 
[cmake] --   GeoDistance               YES 
[cmake] --   GeoMap                    YES 
[cmake] --   VisualSimilarity          YES 
[cmake] -- ======================================
[cmake] -- 
[cmake] -- Configuring done (4.0s)
[cmake] -- Generating done (3.7s)
[cmake] -- Build files have been written to: C:/iVS3D/build/windows-release
```

Don't worry if some components are marked as `NO`, this is expected if you do not have the required dependencies installed. For example, the `NeuralNet` component requires the [Onnxruntime] library to be installed, which is optional.
</details>

This will create the build files in the `build/windows-release` folder. Use `cmake -L ..` to see [all available configurations](#cmake-configuration-options):

Once you are happy with your configuration you can build and install your project by running:

```powershell
cmake --build --preset usr-windows-release --config Release
cmake --install build/windows-release
```

The install step will setup the folder structure and copy the plugin binaries to their appropriate location. By default, the binaries are installed to `build/windows-release/bin` Once both steps finished you can run iVS3D: 
```powershell
.\build\windows-release\bin\iVS3D-core.exe
```
|  |  |
| -- | -- |
| ⚠️ | Make sure the .dll files of OpenCV, Onnxruntime, Ffmpeg, CUDA and CuDNN are all available in the PATH, so they can be found when running iVS3D. |
| | |

## Cmake configuration options
Cmake configuration options are added using the `-D` flag or can be set in the `CMakeUserPresets.json`. Following options are available when configuring your iVS3D build:

| Option         | Type | Default value | Description
| -------------- | ---- | ------------- | -------------
| Build_Plugins  | BOOL | ON            | Enable compilation of plugins
| Build_Tests    | BOOL | OFF           | Compile test suite
| Build_Examples | BOOL | OFF | Compile example applications
| Install_Models | BOOL | ON | Copy onnx models for plugins to install location
| OpenCV_DIR | PATH |  | Path to OpenCVConfig.cmake
| Onnxruntime_ROOT_DIR | PATH |  | Path to Onnxruntime installation
| Ffmpeg_ROOT_DIR | PATH |  | Path to Ffmpeg installation
| Update_Translations | BOOL | OFF | Regenerate translation files from source code (runs `lupdate.exe`)
| With_CUDA | BOOL | ON | Use CUDA to accelerate computations on the GPU. This requires CUDA, CUDNN and an OpenCV build with CUDA support to be installed!


  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [Visual Studio]: <https://visualstudio.microsoft.com/de>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
  [Onnxruntime]: https://github.com/microsoft/onnxruntime
  [Ffmpeg]: https://ffmpeg.org