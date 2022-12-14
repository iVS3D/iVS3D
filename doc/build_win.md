[Documentation](../README.md) / Deploy from source for windows

# Deploy from source for windows
To build the project from source on a windows machine you can use the terminal or download Qt Creator from the official Qt website. If you want to use the Qt Creator follow our instructions [here](build_qtcreator.md). To use the terminal follow the instructions in this document.

[Qt] 5.12 and [MSVC] 2015 have to be installed. Clone the iVS3D repository from our GitHub and download [OpenCV] 4.5.0. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Copy the _3rdparty.txt_ file, rename to 3rdparty.pri and set the paths to OpenCV and CUDA (if needed) there. 

Open the deploy.bat file we provide in the tools folder and set your paths in the variables-section:

| variable     | value                     | required |
|--------------|---------------------------|----------|
| QTDIR        | C:\Qt\5.12.12\gcc_64      | yes      |
| JOM          | C:/Qt/Tools/QtCreator/bin/jom/jom.exe | yes |
| VSDIR        | C:\Program Files (x86)\Microsoft Visual Studio 14.0 | yes |
| PROJ         | C:\Path\to\iVS3D.pro      | yes |
| APP_VERSION  | 1.3.3                     | no |
| APP_DATE     | 2022-12-13                | no |
| USE_CUDA     | 1 or 0                    | yes |
| INSTALL_PATH | path\to\installfolder     | yes, default |

Save the changes and run it to deploy the application. This script will automatically not generate a _3rdparty.pri_ file!

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
