<p align="center">
  <image width="200" height="200" src="doc/images/GUI_ICON_IVS3D_rounded.png" />
</p>

<div align="center">
  <h1>🔍 iVS3D: intelligent Video Sampler 3D </h1>
  <img>
  <a href="https://arxiv.org/abs/2110.11810">
    <img src="https://img.shields.io/badge/arXiv-22110.11810-b31b1b" alt="arXiv">
  </a>
  <a href="https://ivs3d.github.io/iVS3D/">
    <img src="https://img.shields.io/badge/Documentation-blue" alt="Documentation">
  </a>

  [Max Hermann](https://github.com/Max-Hermann), [Dominik Wüst](https://github.com/dom-wuest), [Dominic Zahn](https://github.com/DominicZahn), [Daniel Brommer](https://github.com/dabrommer)

</div>

```bibtex
@article{DBLP:journals/corr/abs-2110-11810,
  author       = {Max Hermann and Thomas Pollok and Daniel Brommer and Dominic Zahn},
  title        = {{IVS3D:} An Open Source Framework for Intelligent Video Sampling and
                  Preprocessing to Facilitate 3D Reconstruction},
  journal      = {CoRR},
  year         = {2021}
}
```

## 📜 Overview
The **i**ntelligent **V**ideo **S**ampler **3D** serves as a preprocessing tool to select suitable images for 3D reconstructions. Its pipeline supports importing photogrammetric data in the form of image sequences or videos, which are complemented by metadata, such as GPS poses.

An extensive range of[plugins](-plugins-) either selects images based on, e.g., visual similarity or sharpness, or creates masks to enhance the provided RGB information further. New algorithms can easily be added by creating new plugins, which are connected to our feature-rich interface.

The resulting image and meta information are exported and can be directly used to start a [3D reconstruction](#-3d-reconstruction) process through the application itself.

![overview](doc/images/iVS3D_overview.svg)

## 🚀 Quick Start
The graphical user interface is split into five different sections. 1. Input, 2. Sampling, 3. Export, 4. Executed steps and 5. Video player with the timeline for selected images.

![GUI-tutorial](doc/images/GUI_overview.png)

This tutorial will guide you through a basic workflow with iVS3D. To follow along, download one of our latest [Ready-To-Use Builds](#ready-to-use-builds-for-windows-and-linux) for Debian, Ubuntu or Windows, or [compile from source](#build-from-source) for your platform. Download a video from the [Tanks and Temples Benchmark](https://www.tanksandtemples.org/), we use the Lighthouse video.

**Step 1: Import and preview**

Run `iVS3D-core` and import the video. This can be done using the `Open Input Video` action in the `File`-menu at the top. Alternatively, you can drag and drop the video into the application. Now you can preview the video:

**Step 2: Select important images**

In the timeline underneath the preview, all 8321 images are marked as selected, which is indicated by the red line. We want to reduce the number of images to speed up the reconstruction, so we use the `Nth image selection`-Plugin to sample down to one image per second. In the `Image selection` tab, select the `Nth image selection` plugin and hit `Start selection`. Now we are down to 277 selected images. To improve the quality of the images, we also run the `Blur detection` plugin. This will replace blurred images with better ones in the neighborhood. This might take a few minutes since we are processing 4K images.

You can see all the steps that were performed in the `Executed steps` tab. There can revert to an older selection of images if you wish. More plugins for automated image selection are available, see [here](#plugins) for a detailed overwiew.

**Step 3: Export selected images**

Once the algorithm is finished, we can export the selected images. In the `Export`-tab select a fitting location and name for this set of images. We choose `export` in the example. You can also change the resolution of the images. To speed things up, we reduced the image resolution to HD and hit export:

![Output-tutorial](doc/images/export_tutorial.png)

**Step 4: Reconstruct 3D scene**
Now the images have been written to the disk. Open your file explorer and navigate to the export location you chose to see the result. We can use the images to create a 3D point cloud with Colmap. For this follow the instructions [here](#3d-reconstruction).

## 🔍 Plugins 🧮

There are currently 8 plugins implemented:

| Plugin | Description | Supports CUDA |
| ------ | ----------- | ------------- |
| [📈 NthFrame](doc/plugins.md#nthframe) | Selects every N-th frame | |
| [👓 Blur Detection](doc/plugins.md#blur-detection) | Avoids blurry images | :white_check_mark: |
| [🌐 GeoDistance](doc/plugins.md#geodistance) | (requires GPS) Selects images based on the distance between their GPS locations | |
| [🌍 GeoMap](doc/plugins.md#geomap) | (requires GPS) Displays an interactive map for the user to select GPS poses manually | |
| [🏎 Smooth Camera Movement](doc/plugins.md#smooth-camera-movement) | Images create a trajectory with constant flight speed | :white_check_mark: |
| [🐌 Stationary Camera Removal](doc/plugins.md#stationary-camera-removal) | Removes images where camera is not moving | :white_check_mark: |
| [🪞 Deep Visual Similarity](doc/plugins.md#deep-visual-similarity) | Find images with the lowest similarity based on their visual embeddings | :white_check_mark: |
| | |
| [🤖 Semantic Segmentation](doc/plugins.md#semantic-segmentation) | Creates binary masks to exclude objects such as vehicles from the reconstruction by using convolutional neural networks for semantic image segmentation | :white_check_mark: |

These plugins show different approaches to enhance information from an image sequence or video by either selecting images or creating additional masks to improve the 3D reconstruction process. See [here](doc/plugins.md) for a detailed description of the above mentioned plugins.

iVS3D is built with an open plugin interface for adding new plugins. So feel free to add your own. See [here](doc/create_plugin.md) for creating your own plugin.

## ⛪ 3D Reconstruction
iVS3D does prepare the data for 3D reconstruction. For now, we do not perform the reconstruction itself. On Windows, iVS3D provides functionality to configure and start [COLMAP] which performs the reconstruction on the prepared data. This saves time and simplifies the reconstruction process. Make sure to install Python 3.9 or later for the reconstruction!

With the latest update, we introduce a seamless integration of [COLMAP] in our software. In the new *Reconstruction* tab you can configure and start colmap reconstructions, view the reconstruction progress, manage the queue and open the finished products.

Reconstruction can be configured to be executed on the local machine or a remote machine such as a GPU server. Further information:
- [local colmap execution](doc/local_colmap_execution.md)

## 📤 Ready to use builds

We provide builds with and without CUDA for multiple platforms and distributions:
| OS | CPU only | CUDA enabled |
|----|----------|--------------|
| 🪟 Windows 10/11 | :white_check_mark: | :white_check_mark: |
| 🐧 Ubuntu 24.04 | :white_check_mark: | :white_check_mark: |
| 🐧 Ubuntu 22.04 | :white_check_mark: | :white_check_mark: |
| 🐧 Debian 12 | :white_check_mark: | :white_check_mark: |

Check the latest release to get a build for your platform!

Note that the CUDA builds support GTX 10xx and RTX series GPUS. Older GPUs or Laptop GPUs might require building iVS3D from sources with an OpenCV and CUDA build for that specific GPU.

To use the included plugin for semantic segmentation you can download the models we used in our paper:
[Link to models]

To use other models, they have to be in the `.onnx` format. In addition, the plug-in requires a file that maps the classes to specific colors.

## 🔨 Build from source
### 🔗 Dependencies

iVS3D and the baseline plugins use:
- [OpenCV] 4.7.0 with contrib modules
- [Qt] Framework 5.15.2

For CUDA support:
- [NVIDIA CUDA Toolkit API] 12.0

For Windows, we use [MSVC] compiler which is shipped with Visual Studio. On Linux, we use [GCC] 10 compiler.

iVS3D uses the cmake build system, which is available in the terminal or in QtCreator. For detailed instructions on building from source see here:
- Build using [linux terminal](doc/build_linux.md)
- Build using [windows terminal](doc/build_win.md)

### 🧪 Tests

To create the test build add ```Build_Tests=ON``` when configuring your build with cmake. 
Now you can run the tests within the Test Result tab in Qt Creator or use `ctest` to run the test suite in your terminal.

[Link to our test data]

## 🎖️ Acknowledgements
Thanks to everyone that helped bring iVS3D to life.
, 

iVS3D was formaly created as part of the "**P**raxis der **S**oftware **E**ntwicklung" modul at the Karlsruhe Institue of Technology in the winter term 2020/21, by Patrick Binder, Lennart Ruck, [Daniel Brommer](https://github.com/dabrommer), [Dominik Wüst](https://github.com/dom-wuest) and [Dominic Zahn](https://github.com/DominicZahn).
It was build in cooperation with Fraunhofer IOSB, Karlsruhe under supervision of [Max Hermann](https://github.com/Max-Hermann) and Thomas Pollok.

Thanks to [Boitumelo Ruf](https://github.com/boitumeloruf) for helping with the transition from qmake to cmake and implementing the `expert_mode`.

## 🔬 Future Work
- [x] Add remote colmap execution for windows
- [x] Add seemless colmap integration for windows
- [ ] Implement metadata reader for gimbal angles
- [ ] Create plugin for FOV overlap calculation

## 📃 Licence

See the [LICENSE](LICENSE) file for details about the license under which this code is made available.

## 📑 Citations
<a id="2">[2]</a> 
Farnebäck (2003).
Two-Frame Motion Estimation Based on Polynomial Expansion.
Proceedings of the SCIA 2003. Lecture Notes in Computer Science, vol 2749. Springer, Berlin, Heidelberg. https://doi.org/10.1007/3-540-45103-X_50

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
