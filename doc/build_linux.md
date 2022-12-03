[Documentation](../README.md) / Build from source for linux

# Build from source for linux
To build the project from source on a linux machine you can use the terminal or download Qt Creator from the official Qt website. If you want to use the Qt Creator follow our windows instructions [here](build_win.md). To use the terminal follow the instructions in this document.

[Qt] 5.15.2 and [Gcc] have to be installed. Clone the iVS3D repository from our GitHub and download [OpenCV] 4.5.0. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Open the deploy.sh file we provide in the tools folder and set your paths in the variables-section:

| variable     | value                     | required |
|--------------|---------------------------|----------|
| QT_PATH      | path/to/Qt/5.15.2/gcc_64  | yes      |
| USE_CUDA     | ```true``` or ```false``` | yes      |
| CUDA_PRI     | path/to/cuda.pri          | if USE_CUDA=```true``` |
| OCV_PRI      | path/to/opencv.pri        | yes      |
| APP_VERSION  | 1.3.2  (example)          | no       |
| APP_DATE     | 2022-25-01  (example)     | no       |
| PRO_PATH     | path/to/iVS3D.pro         | yes, default  |
| INSTALL_PATH | path/to/installfolder     | yes, default  |

Make sure you add the lib folders of OpenCV and Qt to your LD_LIBRARY_PATH above the variables as well!

Save the changes and run it to deploy the application.


  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [Gcc]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
