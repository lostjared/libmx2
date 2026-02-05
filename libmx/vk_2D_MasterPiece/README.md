# 🎮 Liquid Acid Drop

**A Vulkan-powered puzzle game inspired by the Atari 2600 classic *Acid Drop*, built with the MX2 Engine.**

<img width="1280" height="720" alt="Gameplay Screenshot" src="https://github.com/user-attachments/assets/f36c37d8-5ba1-4a9f-9fac-ea73d1474042" />
<img width="1280" height="720" alt="Menu Screenshot" src="https://github.com/user-attachments/assets/a5719b1c-ee37-4444-917d-0f4c5735ca7b" />

## Overview

Liquid Acid Drop is a color-matching puzzle game where players guide falling tri-colored blocks into an 8×18 grid. Match **3 or more** blocks of the same color horizontally, vertically, or diagonally to clear them and score points. The game features GPU-accelerated fragment shader visual effects, persistent high scores, and progressively increasing difficulty.

Built entirely with **Vulkan** for rendering and **SDL2** for windowing/input, this game serves as both a playable title and a showcase for the [libmx2](https://github.com/lostjared/libmx2) engine's 2D sprite and text rendering capabilities.

## Features

- **Vulkan Rendering Pipeline** — All graphics are rendered via Vulkan, including sprite batching, text rendering with SDL_ttf, and custom SPIR-V fragment shaders
- **Shader-Driven Visual Effects** — Backgrounds use kaleidoscope, bubble, and time-warp fragment shaders for psychedelic animated visuals
- **Color-Match Puzzle Mechanics** — Match 3+ blocks horizontally, vertically, or diagonally to clear; cascading gravity triggers chain reactions for bonus points
- **Block Rotation & Color Shifting** — Rotate blocks between vertical and horizontal orientation, and cycle the color order within a piece
- **Animated Match Flashing** — Matched blocks flash before clearing, providing clear visual feedback
- **Progressive Difficulty** — Game speed increases every 10 line clears, with three starting difficulty levels (Easy, Normal, Hard)
- **Persistent High Scores** — Top 10 scores are saved to disk and restored between sessions
- **Resizable Window** — Dynamic font scaling and layout adapts to any window size
- **Fullscreen Support** — Launch in fullscreen mode via command-line flag
- **Cross-Platform** — Builds on Linux, macOS (via MoltenVK), and Windows

## How to Play

### Controls

| Key | Action |
|---|---|
| **← →** | Move block left/right |
| **↓** | Soft drop (move block down faster) |
| **↑** | Shift colors within the block (forward) |
| **Z** | Shift colors within the block (reverse) |
| **Space** | Rotate block (vertical ↔ horizontal) |
| **P** | Pause / Resume |
| **Escape** | Return to main menu / Quit |
| **Enter** | Confirm menu selection |

### Gameplay

1. **Falling Blocks** — A piece made of 3 colored segments falls from the top of the grid
2. **Position & Rotate** — Move the piece left/right, rotate it between vertical and horizontal, and shift the color order to set up matches
3. **Match to Clear** — When 3 or more same-colored blocks align horizontally, vertically, or diagonally, they flash and are cleared from the grid
4. **Gravity & Chains** — After blocks are cleared, remaining blocks fall due to gravity, potentially triggering chain reactions for bonus points
5. **Score Bonuses** — Matching 4 blocks awards +25 bonus points; matching 5+ awards +50 (diagonal matches give +35/+75)
6. **Speed Up** — Every 10 line clears, the drop speed increases
7. **Game Over** — The game ends when blocks stack up to the top of the grid

### Scoring

| Event | Points |
|---|---|
| Each block cleared | +6 |
| 4-block horizontal/vertical match | +25 bonus |
| 5+ block horizontal/vertical match | +50 bonus |
| 4-block diagonal match | +35 bonus |
| 5+ block diagonal match | +75 bonus |

## Building

### Prerequisites

- **C++20** compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- **Vulkan SDK** (or MoltenVK on macOS)
- **SDL2**, **SDL2_ttf**
- **GLM** (OpenGL Mathematics)
- **libpng**, **zlib**
- **CMake** 3.10+
- The **libmx2** library (parent project)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/lostjared/libmx2.git
cd libmx2/libmx

# Configure and build (Vulkan enabled)
mkdir build && cd build
cmake .. -DENABLE_VK=ON
make -j$(nproc)
```

The game binary `vk_2D_MasterPiece` will be produced in the build directory.

### Running

```bash
# Run with default settings (640x480 windowed)
./vk_2D_MasterPiece -p /path/to/vk_2D_MasterPiece

# Custom resolution
./vk_2D_MasterPiece -p /path/to/vk_2D_MasterPiece -w 1280 -h 720

# Fullscreen
./vk_2D_MasterPiece -p /path/to/vk_2D_MasterPiece -f
```

The `-p` flag specifies the path to the game's resource directory (where the `data/` folder and `font.ttf` reside).

## Project Structure

```
vk_2D_MasterPiece/
├── skeleton.cpp          # Game logic, screens, input handling, and main()
├── vk.hpp                # Vulkan engine header (VKWindow, VKSprite, VKText)
├── vk.cpp                # Vulkan engine implementation
├── CMakeLists.txt        # Build configuration
├── font.ttf              # Default game font
├── data/
│   ├── *.png             # Sprite assets (blocks, backgrounds, UI screens)
│   ├── *_vertex.vert     # GLSL vertex shaders
│   ├── *_fragment.frag   # GLSL fragment shaders (kaleidoscope, bubble, warp, etc.)
│   └── *.spv             # Pre-compiled SPIR-V shader binaries
└── scores.dat            # Persistent high score data (generated at runtime)
```

## Technical Details

### Architecture

The game is built on the **MX2 Vulkan framework** (`mx::VKWindow`), which provides:

- **Swap chain management** with automatic recreation on window resize
- **2D Sprite system** (`mx::VKSprite`) — texture loading, batched draw calls, per-sprite custom fragment shaders with uniform parameters
- **Text rendering** (`mx::VKText`) — SDL_ttf font rasterization uploaded as Vulkan textures with dedicated descriptor sets
- **Depth buffering** and proper Vulkan synchronization primitives

### Game Screens

| Screen | Description |
|---|---|
| **Intro** | Splash screen with animated bubble shader; auto-advances after 3 seconds |
| **Start Menu** | Main menu with New Game, High Scores, Options, Credits, and Quit |
| **Game** | Active gameplay with the puzzle grid, score display, and next-piece preview |
| **Game Over** | Displays final score and line count |
| **High Scores** | Top 10 leaderboard with name entry for qualifying scores |
| **Options** | Difficulty selection (Easy / Normal / Hard) |
| **Credits** | Attribution screen |

### Block Colors

The game features **9 distinct block colors**: Yellow, Orange, Light Blue, Dark Blue, Purple, Pink, Gray, Red, and Green — each with its own sprite texture.

## Credits

- **Engine & Game** — [Jared Bruni](https://github.com/lostjared)
- **Framework** — [libmx2 / MX2 Engine](https://github.com/lostjared/libmx2)
- **Inspired by** — *Acid Drop* (Atari 2600, 1992) and *Columns* (Sega, 1990)

## License

This project is part of [libmx2](https://github.com/lostjared/libmx2). See the [LICENSE](../../LICENSE) file for details.

