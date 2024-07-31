# intelligent Video Sampler 3D ![iVS3D-Logo](doc/images/GUI_ICON_IVS3D_mini.png)

![Qt](doc/images/poweredByQt.png) ![OpenCV](doc/images/poweredByOpenCV.png)

>iVS3D is a framework for intelligent pre-processing of image sequences. iVS3D can downsample entire videos to a specific frame rate, as well as resize and crop the individual images. Furthermore, thanks to the modular architecture, developing and integrating plugins with additional algorithms is easy. We provide three plugins as baseline methods that enable an intelligent selection of suitable images and can enrich them with additional information. To filter out images affected by motion blur, we developed a plugin that detects these frames and searches the spatial neighborhood for suitable images as replacements. The second plugin uses optical flow to detect redundant images caused by a temporarily stationary camera. In our experiments, we show how this approach leads to a more balanced image sampling if the camera speed varies, and that excluding such redundant images leads to a time saving of 8.1 % for our sequences.

[Link to paper] submitted for the 16th International Symposium on Visual Computing (ISVC 2021).


## Features

- Import of images and videos (.jpg, .jpeg, .png, ..., .mp4, .mov, ...)
- Drag and drop to import images, videos, and open projects
- Baseline plugins for selecting images:
    - _Nth Frame_ Algorithm (selects every N-th frame)
    - _Stationary Camera Detection_ Algorithm (selects images based on camera movement)
    - _Blur_ Algorithm (avoid blurry images)
    - _Geo Distance_ Algorithm (selects images based on the distance between their camera location. This requires GPS poses.)
    - More _Sampling_-Algorithms can be added using the plugin interface
- Semantic segmentation to mask challenging areas in the input images to prevent these from being processed
    - More _Transformation_-Algorithms can be added using the plugin interface
- Export with user-selected resolution and ROI (_Region of Interest_)
- Optimised [COLMAP] Interface
    - Start a 3D reconstruction with 2 clicks
- GPU processing with [NVIDIA CUDA Toolkit API]
- Use multiple plugins at once with the batch processing
- Can be used in a headless mode
- Supported Platforms: Windows and Linux


![GUI](doc/images/GUI_overview.png)
The graphical user interface is split into five different sections. 1. Input, 2. Sampling, 3. Export, 4. Executed steps and 5. Video player with the timeline for selected images.

## Plugins

There are currently 4 plugins implemented:

| Plugin | Description | Supports CUDA |
| ------ | ----------- | ------------- |
| [NthFrame](#nthframe) | Selects every N-th frame | |
| [Blur Detection](#blur-detection) | Avoids blurry images | |
| [GeoDistance](#geodistance) | (requires GPS) Selects images based on the distance between their GPS camera location | |
| [GeoMap](#geomap) | (requires GPS) Displays an interactive map for the user to select GPS poses manually | |
| [Smooth Camera Movement](#smooth-camera-movement) | | :white_check_mark: |
| [Stationary Camera Removal](#stationary-camera-removal) | Selects images based on camera movement | :white_check_mark: |
| [Deep Visual Similarity](#deep-visual-similarity) | | :white_check_mark: |
| | |
| [Semantic Segmentation](#semantic-segmentation) | | :white_check_mark: |

These plugins show different approaches to enhance information from an image sequence or video by either selecting images or creating additional masks to improve the 3D reconstruction process.
iVS3D is built with an open plugin interface for adding new plugins. So feel free to add your own. See [here](doc/create_plugin.md) for creating your own plugin.

### Image Selection Plugins
All of these plugins follow a subtractive approach.
They receive a list of selected frames and remove those that contain less information than the others.
This can manifest in removing frames that are close to each other by camera position, duration or some entirely different metric.
#### NthFrame
NthFrame is the easiest way to reduce the number of frames used for a 3D reconstruction.
It removes all frames except for every Nth frame.
Consequently, this plugin is the way to go if you want to downsample to a specific FPS count.
This procedure enables NthFrame to be one of the essential tools for preselecting frames before executing more powerful and long-running algorithms.

In the later stages of an image selection workflow, isolated frames can emerge.
This happens, for example, when selecting frames based on camera position.
If the camera is static in some places, only a few frames are selected in a long duration.
NthFrame, however, could remove those remaining isolated frames because of the nature of the algorithm.
To prevent this from happening, we added an additional feature that always keeps those isolated frames.
This feature is activated by default but can be disabled if necessary.

#### Blur Detection
*TBD*
#### GeoDistance
*TBD*
#### GeoMap
*TBD*
#### Smooth Camera Movement

#### Stationary Camera Removal
#### Deep Visual Similarity
To utilize the power of neural networks (NNs)
### Mask Generation Plugins
#### Semantic Segmentation

## 3D Reconstruction
iVS3D does prepare the data for 3D reconstruction. For now, we do not perform the reconstruction itself. On Windows, iVS3D provides functionality to configure and start [COLMAP] which performs the reconstruction on the prepared data. This saves time and simplifies the reconstruction process. Make sure to install Python 3.9 or later for the reconstruction! 

> The next section is Linux only: OTS integration of colmap is not supported on Windows yet!

With the latest update, we introduce a seamless integration of [COLMAP] in our software. In the new *Reconstruction* tab you can configure and start colmap reconstructions, view the reconstruction progress, manage the queue and open the finished products.

Reconstruction can be configured to be executed on the local machine or a remote machine such as a GPU server. Further information:
- [local colmap execution](doc/local_colmap_execution.md)
- [remote colmap execution](doc/remote_colmap_execution.md)

## Getting started
To guide you through a basic workflow with iVS3D, we provide a tutorial that relies on the Linux version. To follow along, download one of our latest [Ready-To-Use Builds](#ready-to-use-builds-for-windows-and-linux) for Debian 11 or Ubuntu 22.04, or [compile from source](#build-from-source) for your platform.

Download a video from the [Tanks and Temples Benchmark](https://www.tanksandtemples.org/), we use the Lighthouse video.

Run `iVS3D-core` and import the video. This can be done using the `Open Input Video` action in the `File`-menu at the top. Alternatively, you can drag and drop the video into the application. Now you can preview the video:

![GUI-tutorial](doc/images/GUI_tutorial.png)

In the timeline underneath the preview, all 8321 images are marked as selected, which is indicated by the red line. We want to reduce the number of images to speed up the reconstruction, so we use the `Nth image selection`-Plugin to sample down to one image per second. In the `Image selection` tab, select the `Nth image selection` plugin and hit `Start selection`. Now we are down to 277 selected images. To improve the quality of the images, we also run the `Blur detection` plugin. This will replace blurred images with better ones in the neighborhood. This might take a few minutes since we are processing 4K images.

You can see all the steps that were performed in the `Executed steps` tab. There can revert to an older selection of images if you wish.

Once the algorithm is finished, we can export the selected images. In the `Export`-tab select a fitting location and name for this set of images. We choose `export` in the example. You can also change the resolution of the images. To speed things up, we reduced the image resolution to HD and hit export:

![Output-tutorial](doc/images/export_tutorial.png)

Now the images have been written to the disk. Open your file explorer and navigate to the export location you chose to see the result. We can use the images to create a 3D point cloud with Colmap. For this follow the instructions [here](doc/remote_colmap_execution.md).


## Ready to use builds for Windows and Linux

We provide builds with and without CUDA for multiple platforms and distributions:
| OS | CPU only | CUDA enabled |
|----|----------|--------------|
| Windows 10/11 | :white_check_mark: | :white_check_mark: |
| Ubuntu 22.04 | :white_check_mark: | :white_check_mark: |
| Debian 11 | :white_check_mark: | :white_check_mark: |
| Debian 12 | :white_check_mark: | :white_check_mark: |

Check the latest release to get a build for your platform!

Note that the CUDA builds support GTX 10xx and RTX series GPUS. Older GPUs or Laptop GPUs might require building iVS3D from sources with an OpenCV and CUDA build for that specific GPU.

To use the included plugin for semantic segmentation you can download the models we used in our paper:
[Link to models]

To use other models, they have to be in the .onnx format. In addition, the plug-in requires a file that maps the classes to specific colors.

## Build from source
### Dependencies

iVS3D and the baseline plugins use:
- [OpenCV] 4.7.0
- [Qt] Framework 5.15.2

For CUDA support:
- [NVIDIA CUDA Toolkit API] 12.0

For Windows, we use [MSVC] compiler which is shipped with Visual Studio. On Linux, we use [GCC] 10 compiler.

The required dependencies can be imported using the _3rdParty.pri_ file. Further information about including dependencies using _.pri_ files can be found [here](doc/3rdparty.md).

### 2.5 ways to build iVS3D
- Open and build with [Qt Creator](doc/build_qtcreator.md)
- Deploy using automated script on [Windows](doc/build_win.md) or [Linux](doc/build_linux.md)
- Build with [cmake](doc/build_cmake.md) using bash or QtCreator (experimental)


## Tests

To create the test build add ```"CONFIG+=test"``` as an qmake argument to your build configuration. 
Now you can run the tests within the Test Result tab in Qt Creator.

[Link to our test data]

## Future Work
- [ ] Add remote colmap execution for windows
- [ ] Add seemless colmap integration for windows

## Licence

see [Licences.txt](Licences.txt)

## Citations

- Knapitsch et al.: _Tanks and Temples Benchmark_ (2017): [website](https://www.tanksandtemples.org/)

## Authors

Patrick Binder, Daniel Brommer, Lennart Ruck, Dominik Wüst, Dominic Zahn

Fraunhofer IOSB, Karlsruhe

Supervisor: Max Hermann & Thomas Pollok

Created as part of PSE at the Karlsruhe Institute of Technology in the winter term 2020/21

  [COLMAP]: https://demuc.de/colmap/
  [OpenCV]: https://github.com/opencv
  [Qt]:     https://www.qt.io
  [MSVC]:   https://www.microsoft.com/de-de/download/details.aspx?id=48159
  [GCC]:    https://gcc.gnu.org
  [Python]: https://www.python.org/downloads/
  [NVIDIA CUDA Toolkit API]:    https://developer.nvidia.com/cuda-zone
  [cuDNN]:  https://developer.nvidia.com/cudnn
  [Link to paper]: https://arxiv.org/abs/2110.11810
  [Link to models]: https://github.com/iVS3D/iVS3D-models
  [Link to our test data]: https://drive.google.com/drive/folders/1hPFtDqQKF9JzBpNTV016unL7awRCsxNj?usp=sharing
