[Documentation](../README.md) / Plugins

# Plugins

## Image Selection Plugins
All of these plugins follow a subtractive approach. They receive a list of selected frames and remove those that are selected by plugin specific algorithms. This can manifest in removing frames that are close to each other by camera position, duration or some entirely different metric. In this way, for example, all blurred images can be removed first and followed by all redundant images.

### NthFrame
NthFrame is the easiest way to reduce the number of frames used for a 3D reconstruction. It removes all frames except for every Nth frame. Consequently, this plugin is the way to go if you want to downsample to a specific FPS count. This plugin enables NthFrame to be one of the essential tools for preselecting frames before executing more powerful and long-running algorithms.

In the later stages of an image selection workflow, isolated frames can emerge. This happens, for example, when selecting frames based on camera position. If the camera is static in some places, only a few frames are selected in a long duration. NthFrame, however, could remove those remaining isolated frames because of the nature of the algorithm. To prevent this from happening, we added an additional feature that always keeps those isolated frames. This feature is activated by default but can be disabled if necessary.

### Blur Detection
Effects such as motion blur can reduce the number of features extracted from an image. Thus such images are not ideal for 3D reconstruction. The Blur Detection Plugin quantifies image sharpness and blurrieness. With this information, blurred images are detected and the plugin selects a sharper image in the neighbourhood to boost reconstruction quality. 

### GeoDistance
This plugin selects images based on their GPS location. For this to work, the GPS data needs to be available as meta data, either included in the EXIF tag of the images or as a separate metadata file. The plugin uses this data to calculate the distance between image pairs and selects images that are further apart than a specified threshold.

### GeoMap
This plugin selects images based on their GPS location. For this to work, the GPS data needs to be available as meta data, either included in the EXIF tag of the images or as a separate metadata file. The images are displayed in an interactive map and the user can select individual images by clicking on the corresponding point on the map. It is possible to draw a polygon on the map to select many images at once. This allows to reconstruct only a certain area, e.g. only use images on one side of a building.

### Optical Flow Plugins
There are currently two Plugins utilizing the concept of estimating camera movement through optical flow. Optical flow describes the apparent movement of an object in the captured scene. By calculating the optical flow between two images we obtain how far and in which directions an object has moved. In general, there is sparse and dense optical flow. Sparse optical flow only returns the estimated movement at some positions in the image, while dense optical flow predicts a globally smoothed movement for every pixel.

The following flowchart illustrates the general algorithm for the implemented plugins.
```mermaid
flowchart LR
    I[Load Image Pair] <--async--> F[Farnebäck 2003] --> FV[Median] --> KS[Frame Selection];
    subgraph Camera Movement Estimation
    F -->FV;
    end
```
The general algorithm consists of three major steps. First, image pairs are loaded. To estimate the camera movement, we use the dense optical flow algorithm from Farnebäck 2003 [[2]](#2) to receive a global prediction for every pixel in the scene. Afterward, this global displacement field is reduced to a single value by calculating the median length of all displacements. This value now represents the change of camera perspective between the two frames.
> Note that this value is still dependent on the image's resolution and, therefore, does not have a measurement unit attached to it.

During the last step, frames are selected based on the previously estimated camera movement. The following plugins currently differ in this step while utilizing identical initial steps.

A parameter that both plugins have in common is the `Sampling Resolution` parameter. When activated, it resizes images to a lower resolution to speed up the calculation of the Farnebäck algorithm. Reducing the resolution, however, can impact the quality of the displacement estimation in special cases, like when cameras are far away from objects or a high amount of detail is necessary. Another feature that speeds up the execution is the buffer, which is utilized in every plugin. The following plugins buffer the already estimated camera movements and write them into the project file. So, if the plugin was already performed once in this project, it will not be recomputed.As a result, it's usually advised to run a plugin once on the whole dataset and then experiment with the provided parameters. > A partial recomputation is necessary if the input frames change. The estimated camera movement is only viable between the provided two frames.

#### Smooth Camera Movement
Using the previously estimated camera movement, this plugin distributes frames evenly. The resulting frames all have roughly the same camera displacement, so the remaining trajectory has uniform camera movement.
> Note that the camera movement includes rotation and translation.

The user can adjust the parameter `Movement Threshold` to define the camera movement, which is the trigger for selecting the next frame.

#### Stationary Camera Removal
In contrast to the [Smooth Camera Movement](#smooth-camera-movement) plugin, this one looks at each frame pair individually and decides if it should be removed. The idea is that only frames that have some camera displacement in relation to the one before are useful and that redundant images can be removed.

To define if a frame is declared stationary, the parameter `Stationary Threshold` can be specified.

$$m_s = median(M) * \frac{\text{Stationary Threshold (in percent)}}{100}$$

If the computed camera movement between the previous and current frame is higher than $m_s$, the frame is selected. Otherwise, it is removed, and the next two frames are compared.

> Note that the parameter is therefore closely tied to the used video and the specific distribution of camera movements. Tweaking this value for different datasets is advised.

### Deep Visual Similarity
Deep Visual Similarity utilizes the power of neural networks (NNs) to find images with the largest possible visual disparity.
The algorithm executes the following steps:
1. Calculate describing feature vectors for every image
2. Group images, using feature vectors, to clusters
3. Choose images closest to centroids as selected frames

Therefore, only two parameters are required to be set by the user. `K` indirectly determines the number of selected frames. It can be thought of as the `N` parameter in the [NthFrame](#nthframe) Plugin.

$$\text{Num. Selected Frames} = \frac{\text{Num. Input Frames}}{K}$$

As a second parameter, the used NN can be selected. This determines how the feature vector is calculated and which dimension is used in clustering. The plugin provides robust support for NNs in `.onnx` format with the prefix `ImageEmbedding`, ensuring compatibility and confidence in the system. See [here](https://github.com/iVS3D/iVS3D-models) for more detailed information.

## Mask Generation Plugins
Additional data about the scene content can be provided to boost 3D reconstruction performance. A prominent example is the detection, localization, and segmentation of dynamic objects, such as vehicles or bystanders in motion. Masks can remove such data.

iVS3D provides a plugin interface to enable users to use existing plugins or create their own.
Plugins receive frames and can generate masks, which can be displayed live in a preview window or exported along with the selected RGB images.
### Semantic Segmentation
In Semantic Segmentation, every pixel of an image is assigned a class or label. Our plugin uses the power of neural networks (NNs) to label every pixel in a given frame and enables users to create their own custom masks through a checkbox matrix. NNs are provided in the `.onnx` format with a text file that describes the available classes and associated colors used for illustration in the preview.

For more information about the models used, see [here](https://github.com/iVS3D/iVS3D-models).