# intelligent Video Sampler 3D ![iVS3D-Logo](doc/GUI_ICON_IVS3D_mini.png)

![Qt](doc/poweredByQt.png) ![OpenCV](doc/poweredByOpenCV.png)

>iVS3D is a framework for intelligent pre-processing of image sequences. iVS3D is able to down sample entire videos to a specific frame rate, as well as to resize and crop the individual images. Furthermore, thanks to the modular architecture, it is easy to develop and integrate plugins with additional algorithms. We provide three plugins as baseline methods that enable an intelligent selection of suitable images and can enrichthem with additional information. To filter out images affected by motion blur, we developed a plugin that detects these frames and also searches the spatial neighbourhood for suitable images as replacements. The second plugin uses optical flow to detect redundant imagescaused by a temporarily stationary camera. In our experiments, we show how this approach leads to a more balanced image sampling if the camera speed varies, and that excluding such redundant images leads to a time saving of 8.1 % for our sequences.

[Link to paper] submitted for the 16th International Symposium on Visual Computing (ISVC 2021).


## Features

- Import of images and videos (.jpg, .jpeg, .png, ..., .mp4, .mov, ...)
- Drag and drop to import images, videos and open projects
- Baseline plugins for sampling:
    - _NthFrame_ Algorithm (selects every N-th frame)
    - _CameraMovement_ Algorithm (selects images based on camera movement)
    - _Blur_ Algorithm (avoid blurry images)
    - More _Sampling_-Algorithms can be added using the plugin interface
- Semantic segmentation to mask challenging areas in the input images to prevent these from being processed
    - More _Transformation_-Algorithms can be added using the plugin interface
- Export with user selected resolution and ROI (_Region of Interest_)
- Optimised [COLMAP] Interface
    - Start of a 3D reconstruction with 2 clicks (project.ini will be generated automatically)
- GPU proccesing with [NVIDIA CUDA Toolkit API]
- Use multiple plugins at once with the batch proccesing
- Can be used in a headless mode
- Supported Plattforms: Windows and Linux

![GUI](doc/gui.png)
Graphical user interface which is split in five different sections. 1. Input, 2. Plugins, 3. Export,
4. Batch processing and 5. Video player with the timeline for keyframes.

## Plugins

There are currently 4 plugins implemented:

| Plugin | Description |
| ------ | ------ |
| NthFrame Plugin | Selects every N-th frame |
| CameraMovement Plugin | Selects images based on camera movement |
| Blur Plugin | Avoids blurry images |
| SemanticSegmentation Plugin | Masks images |

## Dependencies

iVS3D and the baseline plugins use:
- [OpenCV] 4.5.0
- [Qt] Framework 5.9.1

For CUDA support:
- [NVIDIA CUDA Toolkit API] 10.1
- [cuDNN] 8.0

The used compiler is [MSVC] 2015.

The required dependencies can be imported using the _3rdParty.pri_ file.

## Ready to use builds for Windows 10 x64 and Linux

We provide Windows 10 x64 builds with and without CUDA as well as Linux builds with and without CUDA. To use the included plugin for semantic segmentation you can download the models we used in our paper:
[Link to models]

To use other models, they have to be in the .onnx format. In addition, the plug-in requires a file that maps the classes to specific colors.

*compatible with the following CUDA compute capabilities:
| CUDA version 	| GPUs (exemplary) |
|---------------|---------------------|
|	5.0    	|	GeForce GTX 760/770/780, GeForce MX130, GeForce 945M
|	5.2    	|	GeForce GTX 960/970/980
|	6.1	|    	GeForce GTX 1050/1050Ti/1060/1070/1080/1080Ti, GeForce GT 1030
|	7.5	|    	GeForce RTX 2060/2070/2080

## Compile from source without CUDA

[Qt] 5.9.1 Creator and [MSVC] 2015 have to be installed. Clone the intelligent-video-sampler repository.
Download [OpenCV] 4.5.0 and include it using the _3rdParty.pri_ file. Therefore duplicate the 3rdparty.txt inside 
the iVS3D directory and change the file type to .pri. In this file change the path inside the ```include(...)```
to your local OpenCV path. It is recommended to add the OpenCV bin and lib directories to your path variable so the 
compiler and linker can find all necessary dependencies. If OpenCV is not added to the path variable the .dlls need to be
placed in the same directory as the iVS3D-core.exe in order to run it!


Open the iVS3D.pro within QtCreator. Now you can run iVS3D within Qt Creator in debug or release configuration.

To deploy iVS3D as a standalone executable some build steps need to bee added to your build configuration. Therefore open the
Projects tab and select Build under Build & Run.

### Steps on Windows

- clone your Release configuration and choose a name for your deploy configuration
- under the general tab change the build directory to your desired output folder
- under build steps add ```"CONFIG+=with_dependencies"``` as additional argument to the qmake step. This will copy all necessary .dlls for OpenCV to your build folder.
- add a new build step, select make and add ```install``` as make argument. Make sure you have two make steps, the first one without additional arguments!
- add a new build step, select make and add ```clean``` as make argument. This will remove temporary files of the build process.
- add a new build step, select Custom Process Step and enter windeployqt.exe as command. As arguments enter ```%{buildDir}\src\iVS3D-core\release\iVS3D-core.exe```. This will add the necessary .dlls for Qt to your build folder
- add a new build step, select Custom Process Step and enter iVS3D-deploy.bat as command. As arguments enter ```%{buildDir}```. This will remove empty folders and clean up the folder structure by removing folders like release and src.

Now you can deploy iVS3D by clicking the hammer icon in the bottom left corner. The iVS3D-core.exe
can be run outside of qt creator and all necessary dlls from opencv, qt and msvc are located in the same direcototy as the iVS3D-core.exe.

### Steps on Linux

- clone your Release configuration and choose a name for your deploy configuration
- under the general tab change the build directory to your desired output folder
- add a new build step, select make and add ```install``` as make argument. Make sure you have two make steps, the first one without additional arguments!
- add a new build step, select make and add ```clean``` as make argument. This will remove temporary files of the build process.
- add a new build step, select Custom Process Step and enter iVS3D-deploy.sh as command. As arguments enter ```%{buildDir}/iVS3D-core```. Optionally you can enter the path to linuxdeployqt-executable as the second argument and the path to the linux-dependencies.txt file as a third argument. This will remove empty folders and clean up the folder structure by removing folders like release and src. linuxdeployqt adds the necessary .so files from Qt and OpenCV. Additional dependencies from the txt file are added as well.

Now you can deploy iVS3D by clicking the hammer icon in the bottom left corner. The iVS3D-core executable within
can be run outside of qt creator and all necessary .so files from opencv, qt and the system are located in the lib directory.

## Compile from source with CUDA

To use CUDA a CUDA compatible OpenCV version has to be used and included in _3rdParty.pri_ file:

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



## Tests

To create the test build add ```"CONFIG+=test"``` as an qmake argument to your build configuration. 
Now you can run the tests within the Test Result tab in Qt Creator.

[Link to our test data]

## Future Work
- [ ] Make use of provided Meta Data such as GPS Data
- [ ] Speed up export
- [ ] Speed up optical flow calculations

## Licence

see [Licences.txt](Licences.txt)

## Authors

Patrick Binder, Daniel Brommer, Lennart Ruck, Dominik W??st, Dominic Zahn

Fraunhofer IOSB, Karlsruhe

Supervisor: Max Hermann & Thomas Pollok

Created as part of PSE at the Karlsruhe Institut of Technlogy in the winter term 2020/21

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

  [COLMAP]: <https://demuc.de/colmap/>
  [OpenCV]: <https://github.com/opencv>
  [Qt]:     <https://www.qt.io>
  [MSVC]:   <https://www.microsoft.com/de-de/download/details.aspx?id=48159>
  [NVIDIA CUDA Toolkit API]:    <https://developer.nvidia.com/cuda-zone>
  [cuDNN]:  <https://developer.nvidia.com/cudnn>
  [Link to paper]: <https://arxiv.org/abs/2110.11810>
  [Link to models]: <https://drive.google.com/drive/folders/122EDO4UxhEYRy5MI1OIpePnsibwGGXjA?usp=sharing>
  [Link to our test data]: <https://drive.google.com/drive/folders/1hPFtDqQKF9JzBpNTV016unL7awRCsxNj?usp=sharing>
