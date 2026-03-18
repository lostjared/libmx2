# MXMOD Model Viewer

A Qt6-based GUI front end for viewing 3D mesh and model files using either an **OpenGL** or **Vulkan** rendering backend.

<img width="1923" height="1206" alt="Screenshot" src="https://github.com/user-attachments/assets/5f410902-a5c5-4eca-ba6f-1fdd45c0e13c" />

## Overview

MXMOD Model Viewer provides a desktop application for loading, inspecting, and rendering 3D models stored in `.mxmod`, `.mxmod.z` (compressed), and `.obj` formats. The viewer launches an external renderer (`gl_mxmod` for OpenGL or `vk_mxmodel` for Vulkan) and displays its output in a built-in console, giving you full control over the rendering pipeline from a single window.

## Features

- **Dual rendering backend** — choose between OpenGL (`gl_mxmod`) and Vulkan (`vk_mxmodel`) at launch time
- **Supported model formats** — `.mxmod`, `.mxmod.z` (zlib-compressed), and Wavefront `.obj`
- **Texture support** — load `.tex`, `.png`, `.jpg`, `.bmp`, and `.mtl` texture files, with a separate texture directory option
- **Multiple resolutions** — 720p, 1080p, 1440p, and 4K presets
- **Configurable FPS** — set a target frame rate from 30 to 240 fps
- **Optional compression** — toggle zlib compression for model data
- **Drag and drop** — drop model or texture files directly onto the window
- **Recent files** — quick access to previously opened models
- **Real-time console** — timestamped stdout/stderr from the renderer with color-coded errors
- **Persistent settings** — window size, position, paths, and preferences are saved between sessions
- **Custom stylesheet** — ships with a dark theme via `stylesheet.qss`

## Prerequisites

- **Qt 6** (Core, Gui, Widgets)
- **CMake 3.16+**
- **C++20** compatible compiler
- One or both rendering backends installed and available on your `PATH`:
  - `gl_mxmod` — the OpenGL-based model renderer (from the libmx2 project)
  - `vk_mxmodel` — the Vulkan-based model renderer (from the libmx2 project)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

The build will automatically copy the `data/` directory (models, textures, shaders, fonts) alongside the executable.

Alternatively, open `model_viewer.pro` in Qt Creator for a qmake-based workflow.

## Usage

1. Launch the `model_viewer` executable.
2. Select a **Model File** (`.mxmod`, `.mxmod.z`, or `.obj`).
3. Select a **Texture File** (`.tex`, `.png`, `.mtl`, etc.).
4. Select a **Texture Directory** containing additional texture assets.
5. Choose a **Backend** (OpenGL or Vulkan), **Resolution**, and **FPS** target.
6. Click **Launch Viewer**.

The renderer's output streams into the console pane in real time. Use **Stop** to terminate the renderer or **Clear** to reset the console.

### Command-line arguments passed to the renderer

| Flag | Description |
|------|-------------|
| `-m` | Path to the model file |
| `-t` | Path to the texture file |
| `-T` | Path to the texture directory |
| `-r` | Resolution (e.g. `1920x1080`) |
| `--compress` | `true` or `false` — enable zlib compression |
| `-p` | Application directory path |

### Settings

Go to **Help → Settings** to configure a custom path to the renderer executable if it is not on your system `PATH`.

## Included Data

The `data/` directory ships with a collection of sample models and textures:

- Various `.mxmod` models (UFOs, ships, trees, planets, geometric shapes, etc.)
- Texture files in `.tex` and `.png` formats
- Pre-compiled SPIR-V shaders (`.spv`) and GLSL sources for the Vulkan backend
- A TrueType font for in-viewer text rendering

## WebAssembly Build

A web-based build is available under the `web/` directory for running the viewer in a browser via Emscripten.

## License

Part of the [libmx2](https://github.com/lostjared/libmx2) project. See the repository root for license details.

