[Documentation](../README.md) / Build from source with Qt Creator

# Build from source with Qt Creator

## Prerequisites
To build the project from source using Qt Creator please install Qt Framwork from the official Qt website. This ensures the compiler as well as all Qt dependencies are installed and the build environment is setup.

[Qt] 5.15.2 and Visual Studio 2019 or 2022  with [MSVC] 2019 or 2022 on Windows or [GCC] 10 on linux have to be installed. 

Download [OpenCV] 4.7.0 or build it from source. Your OpenCV build should contain a file `OpenCVConfig.cmake`. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Finally, install cmake 3.14 or newer, git and git-lfs.

Clone the iVS3D repository from our GitHub:
```sh
git clone --recursive https://github.com/iVS3D/iVS3D.git
```

## Configure Project in QtCreator
Now you can open the _CMakeLists.txt_ file in QtCreator. If prompted to configure your project, select Qt 5.15.2 for MSVC 2019/2022 (windows) or GCC (linux):

![](images/QtCreator_configure_project.PNG)

Next, go to the `Projects`-tab (left side). At the top, in the 'Build Settings' section, you can select the debug or release configuration or create a new configuration. 

Currently, the cmake configuration fails because we have not yet linked to opencv which is required to build iVS3D.

Switch to the `Current Configuration`-tab and add the following cmake options:
- `OpenCV_DIR` (Directory): Path to OpenCVConfig.cmake file
- `With_CUDA` (Boolean): Use CUDA (on/off)
- `CMAKE_INSTALL_PREFIX` (Directory): location to install to, don't leave default as this could require admin priviledges!

![](images/QtCreator_config.png)

## Build environment
In the next section you can configure your build environment. QtCreator does not automatically include your system environment, so make sure you have the following configurations:
- PATH variable contains:
  - Path to opencv bin folder (windows)
  - Path to Qt bin folder (for running lrelease, both windows and linux)
  - Path to cuda bin folder (both windows and linux)
  - Path to cudnn bin folder (windows, optional)
- LD_LIBRARY_PATH variable contains:
  - Path to cudnn lib folder (linux, optional)

If you encounter any issues with finding dependencies such as cublastLt, libopencv... double check the build environment as this is a common source of errors!

With this configuration, hit `Run CMake`. Now you can configure and compile iVS3D.

## Install and run iVS3D
When running iVS3D within QtCreator you will realize that the plugins are missing. This is because they are compiled as subprojects and the resulting files are not yet placed in plugin directory of iVS3D. This can be resolved by adding cmake install to your run configuration. Go to `Run Settings` and add `CMakeBuild as a new Deploy Step:
![](images/QtCreator_Deployment.png)

Press run again and check that iVS3D has been installed to the location you specified using CMAKE_INSTALL_PREFIX!

QtCreator is still running the executable in your build folder, not the one you just installed. To change that, add a new run configuration in the `Run`-tab below. Select Custom executable and pick iVS3D-core from your install location. Now you can run iVS3D from within QtCreator. Your installation is updated whenever you make changes to the code in QtCreator, so you only need to do this setup once.


  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [GCC]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
