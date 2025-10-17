# Tutorial

This tutorial will guide you through a basic workflow with iVS3D. To follow along, download one of our latest [Ready-To-Use Builds](../README.md#-ready-to-use-builds) for Debian, Ubuntu or Windows, or [compile from source](../README.md#-build-from-source) for your platform. Download a video from the [Tanks and Temples Benchmark](https://www.tanksandtemples.org/), we use the Lighthouse video.

**Step 1: Import and preview**

Run `iVS3D-core` and import the video. This can be done using the `Open Input Video` action in the `File`-menu at the top. Alternatively, you can drag and drop the video into the application. Now you can preview the video:

![GUI-tutorial](images/GUI_overview.png)

The graphical user interface is split into five different sections:
 1. Input
 2. Sampling
 3. Export
 4. Executed steps
 5. Video player with the timeline for selected images.

**Step 2: Select important images**

In the timeline underneath the preview, all 8321 images are marked as selected, which is indicated by the red line. We want to reduce the number of images to speed up the reconstruction, so we use the `Nth image selection`-Plugin to sample down to one image per second. In the `Image selection` tab, select the `Nth image selection` plugin and hit `Start selection`. Now we are down to 277 selected images. To improve the quality of the images, we also run the `Blur detection` plugin. This will remove blurred images. This might take a few minutes since we are processing 4K images.

You can see all the steps that were performed in the `Executed steps` tab. There can revert to an older selection of images if you wish. More plugins for automated image selection are available, see [here](../README.md#-plugins-) for a detailed overwiew.

**Step 3: Export selected images**

Once the algorithm is finished, we can export the selected images. In the `Export`-tab select a fitting location and name for this set of images. We choose `export` in the example. You can also change the resolution of the images. To speed things up, we reduced the image resolution to HD and hit export:

![Output-tutorial](images/export_tutorial.png)

**Step 4: Reconstruct 3D scene**

Now the images have been written to the disk. Open your file explorer and navigate to the export location you chose to see the result. We can use the images to create a 3D point cloud with Colmap. For this follow the instructions [here](../README.md#-3d-reconstruction).