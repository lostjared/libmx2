# Vulkan Pong

A Vulkan implementation of the classic Pong game, ported from the OpenGL version.

## Features

- Full Vulkan rendering pipeline
- 3D cube-based paddles and ball
- AI opponent
- Keyboard and mouse/touch input support
- Wireframe mode toggle
- Grid rotation for 3D viewing effects
- Text overlay for score and FPS display

## Controls

| Key/Input | Action |
|-----------|--------|
| Arrow Up/Down | Move paddle 1 up/down |
| Mouse Motion | Control paddle 1 position |
| W/A/S/D | Rotate the 3D view |
| Q | Reset view rotation |
| R | Reset game (scores and positions) |
| SPACE | Toggle wireframe mode |
| ENTER | Reset camera and view |
| ESC | Quit game |

## Building

The project is built as part of the libmx2 build system:

```bash
cd libmx2/libmx/build
cmake ..
make vk_pong_game
```

## Running

```bash
./vk_pong_game -p /path/to/vk_pong/
```

Or specify resolution:
```bash
./vk_pong_game -p /path/to/vk_pong/ -r 1920x1080
```

## Required Assets

The game requires the following assets in the data path:
- `bg.png` - Background texture
- `font.ttf` - TrueType font for text rendering
- `torus.mxmod` or similar model file - 3D model for game objects
- Compiled SPIR-V shaders: `vert.spv`, `frag.spv`

## Architecture

This Vulkan pong implementation extends the `mx::VKWindow` base class, overriding:
- `initVulkan()` - Initialize Vulkan and load game-specific resources
- `event()` - Handle keyboard, mouse, and touch input
- `proc()` - Game logic update (physics, AI, scoring)
- `draw()` - Render the game objects using the Vulkan pipeline

