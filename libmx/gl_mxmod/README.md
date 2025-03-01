# 3D Model Format Viewer

Model file format (MXMOD)

![image](https://github.com/user-attachments/assets/ef72622c-0de9-40c8-91da-59fab9b1c463)


## Description
Model Viewer is a lightweight OpenGL-based application for loading and rendering 3D models with textures. It provides real-time rotation controls and lighting adjustments.

## Features
- Loads 3D models in `.mxmod` format.
- Compression with mxmod_compress tool
- Supports texture mapping with `.png` textures.
- User-controlled rotation and zooming.
- Adjustable light positioning.
- Runs on both native and WebAssembly environments.

## Dependencies
- OpenGL
- SDL2
- mx library
- Argz library

## Command Line Arguments
The program supports several command-line options to configure its behavior:

| Option | Long Option | Description |
|--------|------------|-------------|
| `-h`   | (None)     | Display help message |
| `-p`   | `path assets path` | Specify assets directory |
| `-m`   | `model model file` | Specify the model file |
| `-t`   | `texture file` | Specify the texture file |
| `-T`   | `texture path` | Specify the path for texture files |

## Controls
- **Arrow Keys**: Toggle rotation around X, Y, and Z axes.
- **A / S**: Zoom in and out.
- **P**: Toggle polygon mode (wireframe vs. solid rendering).
- **Spacebar**: Print rotation values to the console.

## License
This project is licensed under the MIT License.
