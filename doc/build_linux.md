[Documentation](../README.md) / Deploy from source for linux

# Deploy from source for linux

## Prerequisites
Install [Qt] 5.15.2 and [gcc] 10 according to instructions on their official websites. Please add the Qt binary folder to your PATH variable, this is necessary for compiling translations and ui files using tools provided in the Qt installation, e.g.:
```sh
export PATH=$PATH:/home/username/Qt/5.15.2/gcc_64/bin
```

Download [OpenCV] 4.7.0 or build it from source. Your OpenCV build should contain a file `OpenCVConfig.cmake`. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Finally, install cmake 3.14 or newer, git and git-lfs.

## Clone iVS3D

Clone iVS3D recursively to include the neural network models for our plugins.
```sh
git clone --recursive https://github.com/iVS3D/iVS3D.git
cd iVS3D
```

## Configure, build and install with cmake

```sh
mkdir build && cd build
cmake \
  -DOpenCV_DIR="<path to opencv folder containing OpenCVConfig.cmake>" \
  -DWith_CUDA="ON" \
  ..
```
You can disable CUDA by using `-DWith_CUDA="OFF"`. Note that by default, iVS3D will be installed to `/usr/local`, which requires sudo privileges to access. You can change the install location by running:
```sh
cmake -DCMAKE_INSTALL_PREFIX="<path to your prefered install location>" ..
``` 

Use `cmake -L ..` to see [all available configurations](#cmake-configuration-options):

Once you are happy with your configuration you can build and install your project by running:

```sh
cmake --build . --config Release
cmake --install .
```

The install step will setup the folder structure and copy the plugin binaries to their appropriate location. Once both steps finished you can run iVS3D: 
```sh
/usr/local/bin/iVS3D-core
```

## Cmake configuration options
Cmake configuration options are added using the `-D` flag. Following options are available when configuring your iVS3D build:

| Option         | Type | Default value | Description
| -------------- | ---- | ------------- | -------------
| Build_Plugins  | BOOL | ON            | Enable compilation of plugins
| Build_Tests    | BOOL | OFF           | Compile test suite
| CMAKE_BUILD_TYPE | STRING | `Release`   | Use `Debug` to include debug symbols
| CMAKE_INSTALL_PREFIX | STRING | `/usr/local` | install location
| Install_Models | BOOL | ON | Copy onnx models for plugins to install location
| OpenCV_DIR | PATH |  | Path to OpenCVConfig.cmake
| Update_Translations | BOOL | OFF | Regenerate translation files from source code (runs `lupdate`)
| With_CUDA | BOOL | ON | Use CUDA to accelerate computations on the GPU. This requires CUDA, CUDNN and an OpenCV build with CUDA support to be installed!



  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [Gcc]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
