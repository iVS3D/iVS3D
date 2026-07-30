# Headless CLI automation

iVS3D can run a configured sampling and export pipeline without opening the GUI.
This is useful for repeatable batch processing and server environments.

## Command

```bash
iVS3D-core --nogui -a auto-settings.json -i sample.mp4 -m sample.srt -o output -l logs
```

The required arguments are:

- `--nogui`: run without creating the main window.
- `-a, --auto`: path to the headless settings JSON file.
- `-i, --in`: path to an input video or image sequence.
- `-o, --out`: output directory used by export steps.

The optional `-m, --metadata` argument imports metadata files such as SRT, GPX,
or TXT before running the configured steps. It can be passed multiple times, for
example `-m flight.srt -m track.gpx`. If explicit metadata files are provided,
all paths must exist and at least one metadata feature must be detected.

Metadata examples:

```bash
iVS3D-core --nogui -i sample.mp4 -a auto-settings.json -m flight.srt -o output
iVS3D-core --nogui -i sample.mp4 -a auto-settings.json -m track.gpx -o output
iVS3D-core --nogui -i sample.mp4 -a auto-settings.json -m positions.txt -o output
```

The optional `-l, --log` argument writes process logs to the given directory.

## Minimal settings

The settings file contains an ordered `steps` array. This example selects every
tenth input frame and exports the resulting image set as PNG files:

```json
{
  "steps": [
    {
      "type": "selection",
      "plugin": "Nth image selection",
      "settings": {
        "N": 10,
        "KeepIsolated": false
      }
    },
    {
      "type": "export",
      "settings": {
        "format": "png"
      }
    }
  ]
}
```

Headless mode uses English plugin names so settings files are portable across
systems with different GUI locales.

## Step types

### Selection

Selection steps run an image-selection plugin and replace the current selection
with the plugin result.

```json
{
  "type": "selection",
  "plugin": "Nth image selection",
  "settings": {
    "N": 10,
    "KeepIsolated": false
  }
}
```

The `plugin` value must match a plugin that supports image selection. The
`settings` object is passed to the plugin by key.

Examples for the currently supported plugins can be found in [cli-examples](doc/cli-examples/selection/).

### Mask

Mask steps add a mask-generation plugin to the export mask stack.

```json
{
  "type": "mask",
  "plugin": "Segmentation Masks",
  "settings": {
    "selectedModel": {
      "name": "model-name",
      "modelConfigPath": "path/to/model-config.json"
    }
  }
}
```

The exact settings depend on the selected plugin. For segmentation, provide a
model configuration that is available on the machine running iVS3D.

Examples for the currently supported plugins can be found in [cli-examples](doc/cli-examples/mask/).

### Export

Export steps write the current image selection and configured masks.

```json
{
  "type": "export",
  "settings": {
    "path": "output",
    "resolution": "1920x1080",
    "format": "png"
  }
}
```

Export settings are optional. If `path` is omitted, `-o, --out` is used. If
`resolution` is omitted, the original input resolution is used. If `format` is
omitted, PNG files are written.
