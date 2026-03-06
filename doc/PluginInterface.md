# Plugin Interface {#plugin_interface_doc}

This page documents the iVS3D plugin contracts available in namespace `PLUG`
and the visualization data model in namespace `VIS`.

## Overview

The plugin interface module provides:

- a common plugin base interface: `PLUG::IBase`
- specialized extension interfaces:
	- `PLUG::IPreview`
	- `PLUG::IMask`
	- `PLUG::ISelection`
- shared error types:
	- `PLUG::Error`
	- `PLUG::ErrorCode`
- shared visualization data structures used by preview plugins
	- `VIS::Visualization` and related overlay/view types

All plugin APIs are designed for asynchronous execution with Qt signal/slot
integration where needed.

## Base interface

### `PLUG::IBase`

`PLUG::IBase` is the main interface that all iVS3D plugins derive from.
It defines:

- plugin identity (`getName()`)
- settings lifecycle (`getSettingsWidget()`, `getSettings()`, `applySettings()`)
- activation lifecycle (`activate()`, `deactivate()`)
- runtime callbacks (`onInputLoaded()`, `onMetaDataLoaded()`, `onIndexChanged()`, `onSelectedImagesChanged()`)
- communication signals (`updatePreview`, `updateSelectedImages`, `updateProgress`, `encounteredError`)

The settings widget ownership is transferred to the core application via
`std::unique_ptr<QWidget>`.

## Additional extension interfaces

### `PLUG::IPreview`

Provides asynchronous preview generation via:

- `generatePreview(const PreviewData&) -> VisualizationResult`

Use this for overlays and visual feedback in the player.

### `PLUG::IMask`

Provides binary mask generation via:

- `generateMask(const MaskData&) -> MaskResult`

Masks are typically exported together with sampled images.

### `PLUG::ISelection`

Provides keyframe/image selection logic via:

- `selectImages(const SelectionData&, volatile bool& cancelFlag) -> SelectionResult`

Use `cancelFlag` for cooperative cancellation of long-running selection jobs.

## Errors

Plugin-facing operations use `tl::expected<..., PLUG::Error>` for error
reporting.

- `PLUG::ErrorCode` classifies error type.
- `PLUG::Error` carries code + user-visible message.

## Visualization model

Preview rendering is represented by `VIS::Visualization` and related types
(`VIS::View`, `VIS::OverlayItem`, `VIS::RectOverlay`, `VIS::TextOverlay`, `VIS::ImageOverlay`, ...).

These types are collected in Doxygen group @ref Visualization.

Coordinates for overlays are generally expressed in normalized `[0,1]` space
and projected to viewport/image dimensions by iVS3D.
