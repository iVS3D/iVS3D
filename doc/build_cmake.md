[Documentation](../README.md) / Build with cmake

# Build with cmake
To build the project from source using cmake you can use the terminal or download Qt Creator from the official Qt website. In both cases make sure to add the Qt binary directory to your PATH variable, this ensures all Qt packages as well as tools such as lrelease can be found automatically.

[Qt] 5.15.2 and [gcc] 10 (linux) or [Visual Studio] 2019 or later (windows) have to be installed. Clone the iVS3D repository from our GitHub and download [OpenCV] 4.7.0. Your OpenCV build should contain a file `OpenCVConfig.cmake`, we refer to the directory containing this file as the `OpenCV_ROOT` To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

## Building with bash
When using a bash to build iVS3D, navigate to the projects root folder. Then configure your project by running: 

```sh
mkdir build && cd build
cmake \
  -DOpenCV_DIR="<path to opencv folder containing OpenCVConfig.cmake>" \
  -DWith_CUDA="ON" \
  -DCMAKE_INSTALL_PREFIX="install" \
  ..
```
You can disable CUDA by using `-DWith_CUDA="OFF"`. Use `cmake -L ..` to see all available configurations. Once you are happy with your configuration you can build and install your project by running:

```sh
cmake --build . --config Release
cmake --install .
```

The install step will setup the folder structure and copy the plugin binaries to their appropriate location. Once both steps finished you can run iVS3D located in `build/install/bin`.
> Note that on windows you have to add the OpenCV binary path to your PATH variable or copy the binaries to your installation folder! 

## Build with QtCreator

> On windows we recommend to add the OpenCV binary directory to your PATH variable before continuing!

To build and run the project within QtCreator open the CMakeLists.txt located in the iVS3D project folder. After selecting Qt 5.15.2 for your compiler, QtCreator will try to configure the project for you. If you did not add OpenCV to your PATH variable before, this will fail because the OpenCV package can't be located. To resolve this or other errors of your configuration, navigate to the Projects tab. You can add Key-Value pairs to your configuration, make sure to add the following (if missing):

 Key  | Value 
------|------
OpenCV_DIR | `<path to opencv folder containing OpenCVConfig.cmake>`
With_CUDA | OFF
CMAKE_INSTALL_PREFIX | install

Then hit "Apply configuration changes". Now you can compile the project by hitting "Build Project". note that iVS3D does not load any plugins as they have not been installed to the correct location. 

To run iVS3D with all the plugins we need to add some aditional steps to our configuration. For this go to the Projects tab again. Edit the `CMAKE_INSTALL_PREFIX` variable if you want to change the installation location, f.e. you can set it to the build directory you are using.

Navigate to the Run Settings and add a new Build step in the deployment section. You can use the default arguments, this will execute `cmake --build . --target install` which installs your executable and all the plugins to the location you selected as `CMAKE_INSTALL_PREFIX`.

We recommend hitting run to make sure this deployment step is executed and the install location is created. Now we can tell QtCreator where we installed the executable by creating a new Run configuration. Click "Add.." and select "Custom executable". Enter the path to your install location, which is `$CMAKE_INSTALL_PREFIX/bin/iVS3D-core.exe` and set the working directory to this as well. Now hit Run to execute iVS3D with your plugins available!

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [gcc]:    <https://gcc.gnu.org>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [Visual Studio]: <https://visualstudio.microsoft.com/de>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>