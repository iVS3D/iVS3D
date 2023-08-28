[Documentation](../README.md) / Build from source with Qt Creator

# Build from source with Qt Creator
To build the project from source using Qt Creator please install Qt Framwork from the official Qt website. This ensures the compiler as well as all Qt dependencies are installed and the build environment is setup.

[Qt] 5.15.2 and Visual Studio 2019 or 2022  with [MSVC] 2019 or 2022 on Windows or [GCC] 10 on linux have to be installed. Clone the iVS3D repository from our GitHub.

## Compile from source without CUDA
Download [OpenCV] 4.7.0 and include it using the _3rdParty.pri_ file. Therefore duplicate the 3rdparty.txt inside 
the iVS3D directory and change the file type to .pri. In this file change the path inside the ```include(...)```
to your local OpenCV path. It is recommended to add the OpenCV bin and lib directories to your PATH variable on windows and LD_LIBRARY_PATH on linux so the 
compiler and linker can find all necessary dependencies. If OpenCV is not added to the path variable the .dlls need to be
placed in the same directory as the iVS3D-core executable in order to run it!

Open the iVS3D.pro within QtCreator. Now you can run iVS3D within Qt Creator in debug or release configuration.

## Compile from source with CUDA

To use CUDA the [NVIDIA CUDA Toolkit API] needs to be installed. A CUDA compatible [OpenCV] version has to be used and included in _3rdParty.pri_ file:

```sd
with_cuda{
    include([path_to_opencv]/opencv-4.7.0-msvc2019-cuda.pri)
}
```

To build with CUDA support simply add  ```"CONFIG+=with_cuda"``` as an qmake argument to your build steps.

## Multiple configurations 
To make switching between the configuration with and without CUDA, we recommend using the *with_cuda* flag. In the _3rdParty.txt_ file we provide this flag is already set up. Just copy the file and rename to _3rdParty.pri_. Then enter your opencv path with CUDA support in the ```include(...)``` statement within the ```with_cuda{...}```. Also set the path to CUDA_BIN_FILES if needed. In the ```else{...}``` block you can include your opencv build without CUDA support. Now you can switch between two builds by adding or removing ```"CONFIG+=with_cuda"``` as an qmake argument to your build steps.

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [GCC]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
