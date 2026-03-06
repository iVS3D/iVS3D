# Model Configuration Format {#modelconfig_readme}

## Overview

ModelConfig provides a JSON-based configuration format for neural network models performing classification tasks. It is used for various image-to-classification tasks including semantic segmentation, object detection, and other classification-based image processing. The format defines normalization parameters, class information, and model metadata.

**Default Example:** iVS3D-segmentationPlugin

## Configuration Format

### Required Fields

#### mean (array of floats)
Normalization mean values for input image channels (typically RGB).

**Example:**
```json
"mean": [0.485, 0.456, 0.406]
```

**Notes:**
- Must have the same length as `std`
- Common values:
  - ImageNet: `[0.485, 0.456, 0.406]`
  - Zero-centered: `[0.5, 0.5, 0.5]`
  - No normalization: `[0.0, 0.0, 0.0]`

#### std (array of floats)
Normalization standard deviation values for input image channels.

**Example:**
```json
"std": [0.229, 0.224, 0.225]
```

**Notes:**
- Must have the same length as `mean`
- Common values:
  - ImageNet: `[0.229, 0.224, 0.225]`
  - Simple scaling: `[0.5, 0.5, 0.5]`
  - No normalization: `[1.0, 1.0, 1.0]`

#### classes (array of objects)
List of object classes the model can detect.

**Example:**
```json
"classes": [
  {
    "id": 1,
    "name": "person",
    "color": [255, 0, 0]
  },
  {
    "id": 2,
    "name": "car",
    "color": [0, 255, 0]
  }
]
```

**Class Object Fields:**
- `id` (integer, optional): Unique identifier for the class. If omitted, classes are numbered sequentially starting from 0.
- `name` (string, required): Human-readable class name
- `color` (array of 3 integers, optional): RGB color values [0-255] for visualization. Defaults to a random color if omitted.

**Notes:**
- Class IDs must be unique within a configuration
- Duplicate IDs will cause validation errors in ObjectDetectionModelManager

### Optional Fields

#### modelPath (string)
Relative or absolute path to the ONNX model file.

**Example:**
```json
"modelPath": "yolov8n.onnx"
```

**Notes:**
- If omitted, the model manager will look for an `.onnx` file with the same base name as the config file

#### inputAlignment (integer)
Input image dimensions must be multiples of this value.

**Example:**
```json
"inputAlignment": 32
```

**Notes:**
- Default: `1` (no alignment requirement)
- Common values for YOLO models: `32` (due to downsampling layers)
- Images are padded if necessary to meet alignment requirements

#### normalizeInput (boolean)
Whether to normalize input pixel values from [0,255] to [0,1] before applying mean/std normalization.

**Example:**
```json
"normalizeInput": true
```

**Notes:**
- Default: `true` (assumes model expects [0,1] input)
- When `true`, pixels are divided by 255.0 before subtracting mean and dividing by std
- Processing order: `output = ((input / 255.0) - mean) / std` when true, `output = (input - mean) / std` when false

#### applyMeanStd (boolean)
Whether to apply mean/std normalization to the input image.

**Example:**
```json
"applyMeanStd": false
```

**Notes:**
- Default: `true` (applies mean/std normalization)

## Complete Example

```json
{
  "modelPath": "yolov8n-coco.onnx",
  "mean": [0.485, 0.456, 0.406],
  "std": [0.229, 0.224, 0.225],
  "inputAlignment": 32,
  "normalizeInput": true,
  "applyMeanStd": true,
  "classes": [
    {"id": 0, "name": "person", "color": [255, 0, 0]},
    {"id": 1, "name": "bicycle", "color": [0, 255, 0]},
    {"id": 2, "name": "car", "color": [0, 0, 255]},
    {"id": 3, "name": "motorcycle", "color": [255, 255, 0]}
  ]
}
```

## Minimal Example (No Optional Fields)

```json
{
  "mean": [0.0, 0.0, 0.0],
  "std": [1.0, 1.0, 1.0],
  "classes": [
    {"name": "object"}
  ]
}
```

## Usage

### Loading a Config

```cpp
#include <ModelConfig.h>

auto result = MCFG::ModelConfig::loadFromFile("/path/to/config.json");
if (result.has_value()) {
    ModelConfig config = result.value();
    // Use config...
} else {
    qDebug() << "Error:" << result.error().message;
}
```

### Accessing Config Data

```cpp
MCFG::ModelConfig config = ...;

// Normalization parameters
std::vector<float> mean = config.getMean();
std::vector<float> std = config.getStd();

// Model metadata
std::string modelPath = config.getModelPath();
uint alignment = config.getInputAlignment();

// Class information
for (const auto& cls : config.getClasses()) {
    uint id = cls.id;
    QString name = cls.name;
    QColor color = cls.color;
    bool selected = cls.selected;
}
```

## Validation

The `ModelConfig::loadFromFile()` function performs basic JSON parsing validation. Additional validation is performed by the `ObjectDetectionModelManager`:

- **File existence**: Checks if the given `.json` file and the `.onnx` file specified by `modelPath` exist
- **Json parsing**: Validates that the JSON is well-formed and contains required fields
- **Normalization consistency**: Verifies `mean` and `std` have the same length
- **Class ID uniqueness**: Ensures no duplicate class IDs

Invalid configurations are flagged with appropriate error states.

## Error Handling

Parse errors return a `tl::expected<ModelConfig, Error>` with error codes:

- `ConfigFileNotFound`: Config file not found
- `ModelFileNotFound`: ONNX model file specified by `modelPath` not found
- `IoError`: File cannot be opened or read
- `ConfigParseError`: Malformed JSON or missing required fields
- `DuplicateClassId`: Duplicate class IDs found in `classes` array

Example error handling:

```cpp
auto result = MCFG::ModelConfig::loadFromFile("model.json");
if (!result.has_value()) {
    switch (result.error().code) {
        case MCFG::ModelConfig::ErrorCode::IoError:
            qDebug() << "Cannot read file";
            break;
        case MCFG::ModelConfig::ErrorCode::ConfigParseError:
            qDebug() << "Invalid JSON:" << result.error().message;
            break;
        default:
            qDebug() << "Error:" << result.error().message;
    }
}
```
