[Documentation](../README.md) / Deploy from source for linux

# Deploy from source for linux
To build the project from source on a linux machine you can use the terminal or download Qt Creator from the official Qt website. If you want to use the Qt Creator follow our instructions [here](build_qtcreator.md). To use the terminal follow the instructions in this document.

[Qt] 5.15.2 and [gcc] 10 have to be installed. Clone the iVS3D repository from our GitHub and download [OpenCV] 4.7.0. To build with CUDA you need an OpenCV build which supports CUDA. In this case download [NVIDIA CUDA Toolkit API] as well.

Copy the _3rdparty.txt_ file, rename to 3rdparty.pri and set the paths to your OpenCV build there. The file should look something like this:

```sh
with_cuda{
    message(USING OPENCV WITH CUDA)

    # path to OpenCV 4.7.0 with CUDA
    include(/home/user/OpenCV/opencv-4.7.0-gcc10-cuda12/opencv-4.7.0-gcc10-cuda12.pri)
} else {
    message(USING OPENCV WITHOUT CUDA)

    #path to OpenCV 4.7.0 without CUDA
    include(/home/user/OpenCV/opencv-4.7.0-gcc10/opencv-4.7.0-gcc10.pri)
}
```
## Setup the deploy.sh script
Open the deploy.sh file we provide in the tools folder and set your paths in the variables-section.

### Configure [Qt] as follows:
| variable     | default value                 |
|--------------|-------------------------------|
| QT_VERSION   | 5.15.2                        |
| QT_PATH      | $HOME/Qt/$QT_VERSION/gcc_64   |

### Configure [OpenCV] as follows:
```sh
export OCV_BIN=/home/user/OpenCV/opencv-4.7.0-gcc10/lib
```

### Configure [Gcc] as follows:
| variable          | default value  |
|-------------------|----------------|
| GCC_VERSION       | 10.2.1         |
| BUILD_ENVIRONMENT | debian11       |

### Configure [NVIDIA CUDA Toolkit API] as follows:
CUDA support is optional. If you dont want to use it, make sure `CUDA_VERSION` is not defined. Otherwise you can select a CUDA version which matches your OpenCV build like this:
```sh
export CUDA_VERSION=12.0
```
In case you did not use the default install location you might need to change `CUDA_BIN`. The default path is 
```sh
export CUDA_BIN=/usr/local/cuda-$CUDA_VERSION/lib64
```

### Configure install location as follows:
Select the install directory for the finished build by changing `INSTALL_PATH`. The default path is inside the `Releases` directory in your iVS3D repository.

## Run the deploy.sh script
Save the changes and run it to deploy the application.

  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [Gcc]:    <https://gcc.gnu.org>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
