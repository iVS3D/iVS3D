[Documentation](../README.md) / Build from source with Qt Creator

# Build from source with Qt Creator
To build the project from source using Qt Creator please install Qt Framwork from the official Qt website. This ensures the compiler as well as all Qt dependencies are installed and the build environment is setup.

[Qt] 5.15.2 and Visual Studio 2019 or 2022  with [MSVC] 2019 or 2022 on Windows or [GCC] 10 on linux have to be installed. Clone the iVS3D repository from our GitHub.

## Setup dependencies
Download [OpenCV] 4.7.0 and include it using the _3rdparty.pri_ file. Therefore duplicate the 3rdparty.txt inside 
the iVS3D directory and change the file type to .pri. In this file change the path inside the ```include(...)```
to your local OpenCV path. The _3rdparty.pri_ file should look like this:
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
If you are not using CUDA, ignore the `with_cuda{...}` section.

## Configure Project
Now you can open the _iVS3D.pro_ project in QtCreator. If prompted to configure your project, select Qt 5.15.2 for MSVC 2019 (windows) or GCC (linux):
![](QtCreator_configure_project.PNG)

Next, go to the `Projects`-tab (left side). At the top, in the 'Build Settings' section, you can select the debug or release configuration or create a new configuration. In the 'Build Steps' section, add a new build step to run `make install` by clicking 'Add Build Step' -> 'Make'
![](QtCreator_configure_build.PNG)

In the newly added build step, add `install` as a make-argument:

![](QtCreator_configure_install.PNG)

This build step will copy resources such as translations, icons and config files to the build folder and is necessary for the app to execute correctly.

## Link dependencies (windows)
It is recommended to add the OpenCV bin and lib directories to your PATH variable so the 
compiler and linker can find all necessary dependencies. You can do this by changing the 'Environment variables'. We reccomend doing this in QtCreator instead, because it is more flexible when switching between different OpenCV builds, f.e. with and without cuda. To do so, go to the `Projects`-tab. In the 'Build Environment' section you can edit the environment variables. There you can add the path to the OpenCV bin folder to the PATH variable. The OpenCV bin folder contains the .dll files. Usually it is located at 
```sh
<OpenCV folder>\x64\vc16\bin
```

If OpenCV is not added to the PATH variable the .dll files need to be
placed in the same directory as the iVS3D-core executable in order to run it!

## Compile from source without CUDA
Now you can run iVS3D within Qt Creator in debug or release configuration.

## Compile from source with CUDA
To use CUDA the [NVIDIA CUDA Toolkit API] needs to be installed. A CUDA compatible [OpenCV] version has to be used and included in _3rdParty.pri_ file:

```sd
with_cuda{
    include([path_to_opencv]/opencv-4.7.0-msvc2019-cuda.pri)
}
```

To build with CUDA support simply add  ```"CONFIG+=with_cuda"``` as an qmake argument to your build steps:
![](QtCreator_configure_cuda.PNG)
Note that you need to add the path to the correct bin folder to your PATH variable! Otherwise the app might crash when running cuda code! We recommend to duplicate your build configuration and use one configuartion with and another one without cuda.

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [GCC]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
