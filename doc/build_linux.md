# Build on Linux

## Prerequisites

> We build and test iVS3D on Ubuntu 22.04, Ubuntu 24.04 and Debian 12. Thus, for other distributions, the following instructions might need to be adjusted. 

### Build tools (required)
- CMake >= 3.22
- [gcc] >= 11.4
- [git]
- [git-lfs]

```sh
sudo apt update
sudo apt install -y cmake build-essential git git-lfs
```

### Dependencies (required)
- [Qt] == 5.15.2
- [OpenCV] >= 4.11 with contrib modules
- [Ffmpeg] latest stable release

Install [Qt] 5.15.2 according to the instructions on their official website. Please add the Qt binary folder to your PATH variable, this is necessary for compiling translations and ui files using tools provided in the Qt installation, e.g.:
```sh
export PATH=$PATH:/home/username/Qt/5.15.2/gcc_64/bin
```

Download prebuilt [OpenCV] packages or build it from source. Make sure you include the contrib modules as they contain algorithms required by our optical flow plugins and many more. Your OpenCV build should contain a file `OpenCVConfig.cmake`, usually it is located in `lib/cmake/opencv4`. This is your OpenCV_DIR.

> To utilize GPU acceleration in iVS3D, you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well. See the [CUDA acceleration](#cuda-acceleration) section below for more information.

Make sure to install the shared libraries of [Ffmpeg] as iVS3D dynamically links against them.

For the Neural Network based plugins, you need to download [Onnxruntime] 1.18.0 or later GPU (make sure to select the version according to your CUDA version) as well.

## Clone iVS3D

Clone iVS3D recursively to include the neural network models for our plugins.
```sh
git clone --recursive https://github.com/iVS3D/iVS3D.git
cd iVS3D
```

## Configure, build and install with CMake Presets

We utilize CMake Presets to simplify the configuration process. To list the available presets, run:
```sh
cmake --list-presets
```
> You can create your own presets to switch configurations easily. For more information, see the [Custom CMake Presets](#custom-cmake-presets) section below.

Decide which preset you want to use, e.g. `release` or `debug`, and run the following command to configure your build:
```sh
cmake --preset <preset name> \
  -D OpenCV_DIR="<path to opencv folder containing OpenCVConfig.cmake>"
```
<details>
<summary>Expand to see the expected output</summary>

```sh
...
-- 
-- ======================================
--   Project Configuration Summary
-- ======================================
-- Build type:         Release
-- Install prefix:     /home/user/iVS3D/build/linux-release/install
-- 
-- 3rd Party Dependencies:
--   Qt                        YES  (5.15.2)
--   OpenCV                    YES  (4.11.0)
--   Onnxruntime               NO  
--   CUDA                      YES  (12.0)
-- 
-- Components:
--   iVS3D-core                YES  (1.6.1.24)
--   NeuralNet                 NO   (Missing ONNX Runtime)
-- 
-- Plugins:
--   SemanticSegmentation      NO   (Missing NeuralNet)
--   StationaryCameraMovement  YES 
--   SmoothCameraMovement      YES 
--   VisualSimilarity          NO   (Missing NeuralNet)
-- ======================================
-- 
-- Configuring done
-- Generating done
-- Build files have been written to: /home/user/iVS3D/build/linux-release
```
Do not worry if some components are marked as `NO`, this is expected if you do not have the required dependencies installed. For example, the `NeuralNet` component requires the [Onnxruntime] library to be installed, which is optional.
</details>

> Instead of specifying the -D OpenCV_DIR option, you can also set the `OpenCV_DIR` environment variable to the path of your OpenCV installation. This will be used by CMake to find the OpenCVConfig.cmake file.

Then, build and install your configuration:
```sh
cmake --build build/<preset name>
cmake --install build/<preset name>
```
Your iVS3D installation will be located in the `build/<preset name>/bin` folder.

> If you want to use a custom install location, you can specify it in the `CMAKE_INSTALL_PREFIX` option when configuring your build. For example:
```sh
cmake --preset <preset name> \
  -D CMAKE_INSTALL_PREFIX="<path to your prefered install location>"
``` 

Use `cmake -L --preset <preset name>` to see [all available configurations](#cmake-configuration-options):

## CMake configuration options
CMake configuration options are added using the `-D` flag. Following options are available when configuring your iVS3D build:

| Option         | Type | Default value | Description
| -------------- | ---- | ------------- | -------------
| Build_Plugins  | BOOL | ON            | Enable compilation of plugins
| Build_Tests    | BOOL | OFF           | Compile test suite
| Build_Examples | BOOL | OFF | Compile example applications
| CMAKE_BUILD_TYPE | STRING | `Release`   | Use `Debug` to include debug symbols
| CMAKE_INSTALL_PREFIX | STRING | `build/<preset name>/install` | install location
| Install_Models | BOOL | ON | Copy onnx models for plugins to install location
| OpenCV_DIR | PATH |  | Path to OpenCVConfig.cmake
| Update_Translations | BOOL | OFF | Regenerate translation files from source code (runs `lupdate`)
| WITH_CUDA | BOOL | ON | Use CUDA to accelerate computations on the GPU. This requires CUDA, CUDNN and an OpenCV build with CUDA support to be installed!
| Onnxruntime_ROOT_DIR | PATH |  | Optional! Path to the onnxruntime installation. See [Neural network inference using Onnxruntime](#neural-network-inference-using-onnxruntime) for more information.

## Custom CMake Presets
As there are many different configuration options we provide a set of CMake Presets to simplify the configuration process. You can find them in the `CMakePresets.json` file in the root directory of the iVS3D repository. Additionally, you can create your own presets by creating a `CMakeUserPresets.json` file in the same directory. To create a preset which inherits from the linux-debug default, adds a custom install location, and disables CUDA, you can use the following example:

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
      "name": "custom-linux-debug",
      "inherits": "linux-debug",
      "hidden": false,
      "cacheVariables": {
        "CMAKE_INSTALL_PREFIX": "/path/to/custom/install/location",
        "WITH_CUDA": "OFF",
        "Onnxruntime_ROOT_DIR": "/path/to/onnxruntime",
        "OpenCV_DIR": "/path/to/opencv/lib/cmake/opencv4"
      }
    }
  ]
}
```

Also, you can define build presets in the `CMakeUserPresets.json` file. Take a look at the default `CMakePresets.json` file for examples. The build presets can be used to specify additional build flags such as the number of jobs (processors) to use.

## Optional extensions

### CUDA acceleration
To enable CUDA acceleration, you need to install 
- [NVIDIA CUDA Toolkit API] >= 12.0 
- cuDNN >= 8.8 
- OpenCV build with CUDA support
If all three are installed, CMake should automatically detect the CUDA installation and enable CUDA support in iVS3D. If you want to disable CUDA support, you can set the `WITH_CUDA` option to `OFF` when configuring your build.
> Make sure the versions of cuDNN and CUDA are compatible with each other. Also, ensure that your GPU supports the CUDA version you are installing and opencv was built with this cuda version as well.

### Neural network inference using Onnxruntime
To utilize neural networks in iVS3D, you can use the [Onnxruntime] library. This is optional and not required for the basic functionality of iVS3D. If you want to use neural networks, download Onnxruntime-gpu >= 1.18 from the official [GitHub Release](https://github.com/microsoft/onnxruntime/releases). Unzip the folder and specify its installation path using the `Onnxruntime_ROOT_DIR` option when configuring your build.

With this configured correctly, the NerualNet component will be enabled and you can use the SemanticSegmentation and VisualSimilarity plugins.

> Make sure to downlaod the onnxruntime-gpu version, even if you do not have a GPU, as it contains the necessary headers for compilation of the plugins. The CPU version does not contain these headers and will result in compilation errors. Also, ensure that the version of Onnxruntime is compatible with the CUDA version you have installed, if you are using CUDA acceleration (use onnxruntime 1.18.0 for CUDA 12.0).

## References
[Documentation](../README.md)
[Development using VSCode](build_vscode.md)

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [Gcc]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
  [git]:    <https://git-scm.com>
  [git-lfs]: <https://git-lfs.github.com>
  [Onnxruntime]: <https://github.com/microsoft/onnxruntime>
  [CUDNN]: <https://developer.nvidia.com/cudnn>
  [CMake]: <https://cmake.org>
