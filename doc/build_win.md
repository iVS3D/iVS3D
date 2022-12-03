[Documentation](../README.md) / Build from source for windows

# Build from source for windows
To build the project from source on a windows machine we recommend installing Qt Creator from the official Qt website. This ensures the compiler as well as all Qt dependencies are installed and the build environment is setup.

[Qt] 5.15.2 and [MSVC] 2015 have to be installed. Clone the iVS3D repository from our GitHub.

## Compile from source without CUDA
Download [OpenCV] 4.5.0 and include it using the _3rdParty.pri_ file. Therefore duplicate the 3rdparty.txt inside 
the iVS3D directory and change the file type to .pri. In this file change the path inside the ```include(...)```
to your local OpenCV path. It is recommended to add the OpenCV bin and lib directories to your path variable so the 
compiler and linker can find all necessary dependencies. If OpenCV is not added to the path variable the .dlls need to be
placed in the same directory as the iVS3D-core.exe in order to run it!

Open the iVS3D.pro within QtCreator. Now you can run iVS3D within Qt Creator in debug or release configuration.

To deploy iVS3D as a standalone executable some build steps need to bee added to your build configuration. Therefore open the *Projects* tab and select *Build* from the menu *Build & Run*:

1. clone your Release configuration and choose a name for your deploy configuration
1. under the general tab change the build directory to your desired output folder
1. under build steps add ```"CONFIG+=with_dependencies"``` as additional argument to the *qmake* step. This will copy all necessary .dlls for OpenCV to your build folder.
1. add a new build step, select *make* and add ```install``` as make argument. Make sure you run *make* first and *make install* afterwards!
1. add a new build step, select *make* and add ```clean``` as make argument. This will remove temporary files of the build process.
1. add a new build step, select *Custom Process Step* and enter windeployqt.exe as command. As arguments enter ```%{buildDir}\src\iVS3D-core\release\iVS3D-core.exe```. This will add the necessary .dlls for Qt to your build folder
1. add a new build step, select *Custom Process Step* and enter iVS3D-deploy.bat as command. As arguments enter ```%{buildDir}```. This will remove empty folders and clean up the folder structure by removing folders like release and src.

Now you can deploy iVS3D by clicking the hammer icon in the bottom left corner. The iVS3D-core.exe and all necessary dlls from opencv, qt and msvc are installed to the output folder you selected in step 2.

## Compile from source with CUDA

To use CUDA the [NVIDIA CUDA Toolkit API] needs to be installed a CUDA compatible [OpenCV] version has to be used and included in _3rdParty.pri_ file:

```sd
with_cuda{
    include([path_to_opencv]/opencv-4.5.0-msvc2015-cuda.pri)
}
```

To copy all CUDA .dll files to the build folder, the .dll files have to be included in the _3rdParty.pri_ file as well:

```sd
with_cuda {
    CUDA_BIN_PATH = C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v10.1\bin
    CUDA_BIN_FILES = *.dll
}
```

To build with CUDA support simply add  ```"CONFIG+=with_cuda"``` as an qmake argument to your build steps.

## Multiple configurations 
To make switching between the configuration with and without CUDA, we recommend using the *with_cuda* flag. In the _3rdParty.txt_ file we provide this flag is already set up. Just copy the file and rename to _3rdParty.pri_. Then enter your opencv path with CUDA support in the ```include(...)``` statement within the ```with_cuda{...}```. Also set the path to CUDA_BIN_FILES if needed. In the ```else{...}``` block you can include your opencv build without CUDA support. Now you can switch between thwe builds by adding or removing ```"CONFIG+=with_cuda"``` as an qmake argument to your build steps.

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
