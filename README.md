<div align="center">
  <img width="200" height="200" src="iVS3D.png" />
</div>

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
The **Intelligent Video Sampler 3D** serves as a preprocessing tool to select suitable images for 3D reconstructions. Its pipeline supports importing photogrammetric data in the form of image sequences or videos, which are complemented by metadata, such as GPS poses.

An extensive range of [plugins](#-plugins-) either selects images based on, e.g., visual similarity or sharpness, or creates masks to enhance the provided RGB information further. New algorithms can easily be added by creating new plugins, which are connected to our feature-rich interface.

The resulting image and meta information are exported and can be directly used to start a [3D reconstruction](#-3d-reconstruction) process through the application itself.

![overview](doc/images/iVS3D_overview.png)

## 🚀 Quick Start
TODO: VIDEO HERE
![GUI-tutorial](doc/images/GUI_overview.png)

To get started quickly, download a [prebuilt binary](#-ready-to-use-builds) from our releases and follow our [tutorial](doc/tutorial.md).

## 🔍 Plugins 🧮

There are currently 8 plugins implemented:

| Plugin | Description | Supports CUDA |
| ------ | ----------- | ------------- |
| [📈 NthFrame](doc/plugins.md#nthframe) | Selects every N-th frame | |
| [👓 Blur Detection](doc/plugins.md#blur-detection) | Avoids blurry images | ✅ |
| [🌐 GeoDistance](doc/plugins.md#geodistance) | (requires GPS) Selects images based on the distance between their GPS locations | |
| [🌍 GeoMap](doc/plugins.md#geomap) | (requires GPS) Displays an interactive map for the user to select GPS poses manually | |
| [🏎 Smooth Camera Movement](doc/plugins.md#smooth-camera-movement) | Images create a trajectory with constant flight speed | ✅ |
| [🐌 Stationary Camera Removal](doc/plugins.md#stationary-camera-removal) | Removes images where camera is not moving | ✅ |
| [🪞 Deep Visual Similarity](doc/plugins.md#deep-visual-similarity) | Find images with the lowest similarity based on their visual embeddings | ✅ |
| [🤖 Semantic Segmentation](doc/plugins.md#semantic-segmentation) | Creates binary masks to exclude objects such as vehicles from the reconstruction by using convolutional neural networks for semantic image segmentation | ✅ |

These plugins show different approaches to enhance information from an image sequence or video by either selecting images or creating additional masks to improve the 3D reconstruction process. See [here](doc/plugins.md) for a detailed description of the above mentioned plugins.

iVS3D is built with an open plugin interface for adding new plugins. So feel free to add your own. See [here](doc/create_plugin.md) for creating your own plugin.

## ⛪ 3D Reconstruction
iVS3D does prepare the data for 3D reconstruction. Afterwards, commonly used tools like [COLMAP] and [OpenMVS] can be used to perform the reconstruction and meshing. We provide a seamless integration of both in our software. In the *Reconstruction* tab you can configure desired products such as a dense point cloud or a textured mesh. iVS3D starts the reconstruction with your configuration based on the exported images and masks, either locally on your machine or on a remote server. You can track the progress, view logs and manage the reconstruction jobs all within the *Reconstruction* tab. For further information see:
- [local reconstruction](doc/reconstruction_local.md)
- [remote reconstruction](doc/reconstruction_remote.md)

## 📤 Ready to use builds

We provide builds with and without CUDA for multiple platforms and distributions:
| OS | CPU only | CUDA enabled |
|----|----------|--------------|
| 🪟 Windows 11 | ✅ | ✅ |
| 🐧 Ubuntu 24.04 | ✅ | ✅ |
| 🐧 Ubuntu 22.04 | ✅ | ✅ |
| 🐧 Debian 12 | ✅ | ✅ |

Check the latest release to get a build for your platform!

Note that the CUDA builds support RTX series GPUs. Older models or Laptop GPUs might require building iVS3D from sources with an OpenCV and CUDA build for that specific GPU.

To use the included plugin for semantic segmentation you can download the models we used in our paper:
[Link to models]

To use other models, they have to be in the `.onnx` format. In addition, the plug-in requires a file that maps the classes to specific colors.

## 🔨 Build from source
### 🔗 Dependencies

iVS3D and the baseline plugins use:
- [OpenCV] 4.11.0 with contrib modules
- [Qt] Framework 5.15.2
- [Ffmpeg] latest stable release
- [Onnxruntime] 1.18.0 GPU (if using CUDA, make sure to select the version according to your CUDA version)

For CUDA support:
- [NVIDIA CUDA Toolkit API] 12.0 (or 12.8)
- [cuDNN] 8.9.0 (for CUDA 12.x)

For Windows, we use [MSVC] 2022 compiler which is shipped with Visual Studio. On Linux, we use [GCC] 11 compiler.

iVS3D uses the cmake build system and utilizes cmake presets. For detailed instructions on building from source see here:
- [linux](doc/build_linux.md)
- [windows](doc/build_win.md)
- [vscode setup](doc/develop_vscode.md)

### 🧪 Tests

To create the test build add ```Build_Tests=ON``` when configuring your build with cmake. 
Now you can run the tests within the Test Result tab in Qt Creator or use `ctest` to run the test suite in your terminal.

[Link to our test data]

## 🎖️ Acknowledgements
Thanks to everyone that helped bring iVS3D to life.
, 

iVS3D was formaly created as part of the **Praxis der Software Entwicklung** modul at the Karlsruhe Institue of Technology in the winter term 2020/21, by Patrick Binder, Lennart Ruck, [Daniel Brommer](https://github.com/dabrommer), [Dominik Wüst](https://github.com/dom-wuest) and [Dominic Zahn](https://github.com/DominicZahn).
It was build in cooperation with Fraunhofer IOSB, Karlsruhe under supervision of [Max Hermann](https://github.com/Max-Hermann) and Thomas Pollok.

Thanks to [Boitumelo Ruf](https://github.com/boitumeloruf) for helping with the transition from qmake to cmake and implementing the `expert_mode`.

## 🔬 Future Work
- [x] Add remote colmap execution for windows
- [x] Add seemless colmap integration for windows
- [ ] Implement metadata reader for gimbal angles
- [ ] Create plugin for FOV overlap calculation

## 📃 Licence

See the [LICENSE](LICENSE) file for details about the license under which this code is made available.

  [COLMAP]: https://demuc.de/colmap/
  [OpenMVS]: https://github.com/cdcseacave/openMVS
  [OpenCV]: https://github.com/opencv
  [Onnxruntime]: https://github.com/microsoft/onnxruntime
  [Ffmpeg]: https://ffmpeg.org
  [Qt]:     https://www.qt.io
  [MSVC]:   https://www.microsoft.com/de-de/download/details.aspx?id=48159
  [GCC]:    https://gcc.gnu.org
  [Python]: https://www.python.org/downloads/
  [NVIDIA CUDA Toolkit API]:    https://developer.nvidia.com/cuda-zone
  [cuDNN]:  https://developer.nvidia.com/cudnn
  [Link to paper]: https://arxiv.org/abs/2110.11810
  [Link to models]: https://github.com/iVS3D/iVS3D-models
  [Link to our test data]: https://drive.google.com/drive/folders/1hPFtDqQKF9JzBpNTV016unL7awRCsxNj?usp=sharing
