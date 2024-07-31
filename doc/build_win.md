[Documentation](../README.md) / Deploy from source for windows

# Deploy from source for windows

## Prerequisites
[Qt] 5.15.2 and [Visual Studio] 2019 or 2022 have to be installed. Make sure to select [MSVC] 2019 or later on both installations. Please add the Qt binary folder to your PATH variable, this is necessary for compiling translations and ui files using tools provided in the Qt installation, e.g.:
```powershell
$env:PATH += ";C:\Qt\MSVC2019\bin"
```

Download [OpenCV] 4.7.0 or build it from source. Your OpenCV build should contain a file `OpenCVConfig.cmake`. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Finally, install cmake 3.14 or newer, git and git-lfs.

## Clone iVS3D

Clone iVS3D recursively to include the neural network models for our plugins.
```powershell
git clone --recursive https://github.com/iVS3D/iVS3D.git
Set-Location -Path iVS3D
```

## Configure, build and install with cmake

```powershell
New-Item -Path "build" --ItemType Directory
Set-Location -Path build
cmake \
  -DOpenCV_DIR="<path to opencv folder containing OpenCVConfig.cmake>" \
  -DWith_CUDA="ON" \
  ..
```
You can disable CUDA by using `-DWith_CUDA="OFF"`. Note that by default, iVS3D will be installed to `C:\Program Files`, which requires admin privileges to access. You can change the install location by running:
```powershell
cmake -DCMAKE_INSTALL_PREFIX="<path to your prefered install location>" ..
``` 

Use `cmake -L ..` to see [all available configurations](#cmake-configuration-options):

Once you are happy with your configuration you can build and install your project by running:

```powershell
cmake --build . --config Release
cmake --install .
```

The install step will setup the folder structure and copy the plugin binaries to their appropriate location. Once both steps finished you can run iVS3D: 
```powershell
iVS3D-core.exe
```

## Cmake configuration options
Cmake configuration options are added using the `-D` flag. Following options are available when configuring your iVS3D build:

| Option         | Type | Default value | Description
| -------------- | ---- | ------------- | -------------
| Build_Plugins  | BOOL | ON            | Enable compilation of plugins
| Build_Tests    | BOOL | OFF           | Compile test suite
| CMAKE_BUILD_TYPE | STRING | `Release`   | Use `Debug` to include debug symbols
| CMAKE_INSTALL_PREFIX | STRING | `C:\Program Files` | install location
| Install_Models | BOOL | ON | Copy onnx models for plugins to install location
| OpenCV_DIR | PATH |  | Path to OpenCVConfig.cmake
| Update_Translations | BOOL | OFF | Regenerate translation files from source code (runs `lupdate.exe`)
| With_CUDA | BOOL | ON | Use CUDA to accelerate computations on the GPU. This requires CUDA, CUDNN and an OpenCV build with CUDA support to be installed!


  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [Visual Studio]: <https://visualstudio.microsoft.com/de>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>