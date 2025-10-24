# Create a plugin
Plugins are a simple way to add functionality to iVS3D. We provide the `IAlgorithm` interface for adding new sampling strategies. In the following, we are going to create a new plugin and look at the functions that need to be implemented. 

### Create a new plugin subproject on Windows
We provide a tool to get you setup as quickly as possible. Therefore, run the *tools/createSamplingPlugin.bat* and pass the `NAME` of your new plugin as an argument.

For example, we create a plugin to randomly pick some keyframes with a probability specified by the user. We'll call the plugin _RandomPicker_, so we run:

```shell session
$ tools/createSamplingPlugin.bat RandomPicker
```

### Create a new plugin subproject on Linux
We provide a tool to get you setup as quickly as possible. Therefore, run the *tools/createSamplingPlugin.sh* and pass the `NAME` of your new plugin as an argument.

For example, we create a plugin to randomly pick some keyframes with a probability specified by the user. We'll call the plugin _RandomPicker_, so we run:

```shell session
$ tools/createSamplingPlugin.sh RandomPicker
```

### Plugin structure
The new plugin has been created in the iVS3D/src directory. A new folder iVS3D-`NAME`Plugin has been created, in our example the folder is called iVS3D-RandomPickerPlugin.

This folder contains:
| File                      | Content                                  |
| ------------------------- | ---------------------------------------- |
| CMakeLists.txt            | The configuration file for building      |
| `NAME`.h                  | The Header file for your plugin          |
| `NAME`.cpp                | The Source code for your plugin          |
| resources.qrc             | resources (such as translations)         |
| translations/`NAME`_de.ts | german translation                       |
| translations/`NAME`_en.ts | english translation                      |

### Build iVS3D with the plugin
Follow the build instructions for your platform to compile iVS3D, this will now also compile the new plugin.

If the Qt version (we use 5.15.2) and iVS3D version of your new build match an existing iVS3D build, then you can simply copy the `NAME`.dll (or `NAME`.so on linux) of the new plugin to the other builds plugin folder to use it with this build as well.
