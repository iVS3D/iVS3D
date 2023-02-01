[Documentation](../README.md) / Create a plugin

# Create a plugin
Plugins are a simple way to add functionality to iVS3D. We provide the `IAlgorithm` interface for adding new sampling strategies. In the following we are going to create a new plugin and look at the functions that need to be implemented. 

### Create a new plugin subproject on Windows
We provide a tool to get you setup as quickly as possible. Therfore run the *tools/createSamplingPlugin.bat* and pass the `NAME` of your new plugin as argument.

For example we create a plugin to randomly pick some keyframes with a probability specified by the user. We'll call the plugin _RandomPicker_, so we run:

```shell session
$ tools/createSamplingPlugin.bat RandomPicker
```

### Create a new plugin subproject on Linux
We provide a tool to get you setup as quickly as possible. Therfore run the *tools/createSamplingPlugin.sh* and pass the `NAME` of your new plugin as argument.

For example we create a plugin to randomly pick some keyframes with a probability specified by the user. We'll call the plugin _RandomPicker_, so we run:

```shell session
$ tools/createSamplingPlugin.sh RandomPicker
```

### Plugin structure
The new plugin has been created in the iVS3D/src directory. A new folder iVS3D-`NAME`Plugin has been created, in our example the folder is called iVS3D-RandomPickerPlugin.

This folder contains:
| File                      | Content                                  |
| ------------------------- | ---------------------------------------- |
| iVS3D-`NAME`Plugin.pro    | The Project file with build instructions |
| `NAME`.h                  | The Header file for your plugin          |
| `NAME`.cpp                | The Source code for your plugin          |
| resources.qrc             | resources (such as translations)         |
| translations/`NAME`_de.ts | german translation                       |
| translations/`NAME`_en.ts | english translation                      |

### Build iVS3D with the plugin
Open the iVS3D.pro file as usual.
> On Windows make sure to run lrelease to create the translations for the new plugin, as lrelease in the Qt creator does not work properly with subdir-projects, so you have to use the lrelease script manually for each plugin. With linux this is not an issue as the qmake step will create the translation files.

Next you can run the build step and your new plugin will be compiled and deployed to the plugin folder.

If the Qt version and iVS3D version of your new build match with another existing iVS3D build, then you can simply copy your plugins `NAME`.dll to the other builds plugin folder to use it with this build as well.

### Compile the plugin for an existing iVS3D build
To build the Plugin we open the src/iVS3D-`NAME`Plugin/iVS3D-`NAME`Plugin.pro file with qt creator. You should be using Qt 5.12 for compatibility with our prebuilds, however if you also build the core yourself you should go with the same version of qt for both.

> On windows make sure you create the translations by running Extras/Extern/Linguist/lrelease in Qt Creator! 

Next we can run qmake step to setup the build folder. However you will not be able to build the plugin right away as the iVS3D-pluginInterface is missing. Form your existing iVS3D build, copy the iVS3D-pluginInterface.lib to your build folder. Now run your build step again to compile your plugin. It will be placed in the plugins-folder outside your build folder.

To use the plugin with the existing iVS3D-build, copy the `NAME`.dll file you just created from the plugins folder to the plugin folder inside your iVS3D-build.