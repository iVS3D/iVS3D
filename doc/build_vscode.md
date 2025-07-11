# Develop using VSCode

To simplify the setup of the iVS3D development environment, we use CMake Presets. We recommend creating a custom `CMakeUserPresets.json` file in the root directory of the iVS3D repository. This file will contain your custom presets and can be used to configure your build environment.

As a starting point you can use something like this:

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 22,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "cuda-linux-release",
            "displayName": "CUDA Linux Release",
            "description": "Linux build in Release mode with CUDA and Onnxruntime enabled",
            "inherits": "linux-release",
            "cacheVariables": {
                "OpenCV_DIR": "/home/user/opencv_4.11.0_contrib_cuda12.0/lib/cmake/opencv4",
                "Onnxruntime_ROOT_DIR": "/home/user/onnxruntime-linux-x64-gpu-cuda12-1.18.0/onnxruntime-linux-x64-gpu-1.18.0"
            }
        },
        {
            "name": "cuda-linux-debug",
            "displayName": "CUDA Linux Debug",
            "description": "Linux build in Debug mode with CUDA and Onnxruntime enabled",
            "inherits": "linux-debug",
            "cacheVariables": {
                "OpenCV_DIR": "/home/user/opencv_4.11.0_contrib_cuda12.0_debug/lib/cmake/opencv4",
                "Onnxruntime_ROOT_DIR": "/home/user/onnxruntime-linux-x64-gpu-cuda12-1.18.0/onnxruntime-linux-x64-gpu-1.18.0"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "build-cuda-linux-release",
            "displayName": "CUDA Linux Release (Build)",
            "configurePreset": "cuda-linux-release",
            "jobs": 12
        },
        {
            "name": "build-cuda-linux-debug",
            "displayName": "CUDA Linux Debug (Build)",
            "configurePreset": "cuda-linux-debug",
            "jobs": 12
        }
    ]
}
```

Note that different OpenCV_DIRs are used in the debug and release presets! Once your `CMakeUserPresets.json` file is set up, you can open the iVS3D project in VSCode and use the CMake Presets to configure your build environment.

## References
[Documentation](../README.md)
[Build on Linux](build_linux.md)