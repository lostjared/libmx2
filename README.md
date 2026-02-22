# libmx2

<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/bb66d9fa-5dbc-4b72-abfc-0f95030c60fc" />
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/38dfd566-24bc-4456-8d5c-fe51930504a0" />

![planet xeon 3](https://github.com/user-attachments/assets/dc9e0b50-d57a-4bc3-a02f-3b61ab9a6d7a)
![gl_breakout3](https://github.com/user-attachments/assets/c5b454da-f2f9-40f6-be85-4cbf33fe78eb)
![vlcsnap-2025-04-04-16h54m33s924](https://github.com/user-attachments/assets/9bf63d65-1318-49a9-9104-db3d506b0082)
![Screenshot 2025-05-04 202711](https://github.com/user-attachments/assets/d2a3a2d1-745d-435a-82a3-2cfbe2f1d971)
<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/afe3d99f-daa5-4b25-bb3f-12aaec12ee18" />


# Tech Specifications
https://lostsidedead.biz/libmx2/mx.html

libmx2 is a cross-platform library that facilitates cross-platform SDL2/OpenGL development using C++20.
It provides a collection of utilities and abstractions to streamline the creation of multimedia applications.

## Motivation

The motivation for creating this library was to give me an easy way to assemble new SDL2 applications 
while using an approach I prefer, an Object Oriented Design. I have experimented with AI in some of the example projects for this library.

## Features

- **SDL2 Integration**: Simplifies the setup and management of SDL2 components.
- **Modern C++20 Design**: Utilizes C++20 features for cleaner and more efficient code.
- **Modular Architecture**: Offers a flexible structure to accommodate various project needs.
- **Optional OpenGL/GLAD/GLM support**: You can compile in support for these libraries with examples
- **Optional Vulkan**: You can compile in support for Vulkan or MoltenVK

## Modular Library Design

libmx2 is split into three separate library modules that are built independently:

- **mx** — The core module. Provides SDL2 window management, input handling, font rendering, PNG/JPEG loading, audio (SDL2_mixer), configuration, and general utilities. This module has no dependency on any graphics API and is always built.
- **mxgl** — The OpenGL module. Contains OpenGL/GLAD initialization, GL shader helpers, 3D model loading, and the in-app console overlay. Built only when `-DOPENGL=ON` (the default). Links against the core `mx` library.
- **mxvk** — The Vulkan module. Contains Vulkan initialization (via volk), Vulkan pipeline/shader helpers, Vulkan-based model rendering, text rendering, and sprite batching. Built as a static library only when `-DVULKAN=ON`. Links against the core `mx` library.

This separation means you can build a project that uses only the core SDL2 features, or opt in to OpenGL, Vulkan, or both. OpenGL and Vulkan never depend on each other — they share only the common `mx` core.
## Getting Started

### Prerequisites

- C++20 compatible compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+)
- SDL2, SDL2_ttf, SDL2_mixer, libpng, zlib installed
- Optional: OpenGL/GLM (includes GLAD) or Vulkan/MoltenVK

### Quick Compile

```bash
./compile.pl
```

then run with
```bash
./run program_name
```

or

From the repo root directory (`libmx2/`), run:

```bash
mkdir build && cd build
cmake -B . -S ../libmx
make -j$(nproc)
```

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/lostjared/libmx2.git
   cd libmx2
   ```

2. Create a build directory and compile:

   ```bash
   mkdir build && cd build
   cmake -B . -S ../libmx
   make -j$(nproc)
   sudo make install
   ```

3. Build with Vulkan support (no OpenGL):

   ```bash
   mkdir build && cd build
   cmake -B . -S ../libmx -DVULKAN=ON -DOPENGL=OFF 
   make -j$(nproc)
   sudo make install
   ```

or

````bash
cmake -B . -S ../libmx -DVULKAN=ON -DOPENGL=OFF
make -j$(nproc)
````

4. Build as a static library:

   ```bash
   mkdir build && cd build
   cmake -B . -S ../libmx -DBUILD_STATIC_LIB=ON
   make -j$(nproc)
   sudo make install
   ```

To do a clean rebuild, remove the build directory first:

   ```bash
   rm -rf build && mkdir build && cd build
   cmake -B . -S ../libmx
   make -j$(nproc)
   ```
Current Options:

* -DJPEG=OFF - optionally compile in support for JPEG images with jpeglib ON or OFF
* -DBUILD_STATIC_LIB=OFF - optionally compile library as a static library ON or OFF
* -DOPENGL=ON optionally compile OpenGL support into the library either ON or OFF
* -DMIXER=ON optionally compile audio support with SDL2_mixer either ON or OFF
* -DVULKAN=ON optionally compile Vulkan support into the library ON  or OFF
* -DMOLTEN=ON optionally compile with MoltenVK
* -DMOLTEN_PATH=/usr/local/opt/molten-vk  (or path you installed molten-vk)

# WebAssembly Demos

https://lostsidedead.biz/Asteroids3D

https://lostsidedead.biz/acidcam.web

https://lostsidedead.biz/acidcam.webgl

https://lostsidedead.biz/game/3DBreak

https://lostsidedead.biz/Breakout

https://lostsidedead.biz/3DPong

https://lostsidedead.biz/Space

https://lostsidedead.biz/game/Space3D

https://lostsidedead.biz/asetroids

https://lostsidedead.biz/Blocks

https://lostsidedead.biz/KnightsTour

https://lostsidedead.biz/point-sprites

https://lostsidedead.biz/point-sprites/stars3d.html

https://lostsidedead.biz/Phong

https://lostsidedead.biz/Torus

https://lostsidedead.biz/Light

https://lostsidedead.biz/Shadow

https://lostsidedead.biz/Skybox

https://lostsidedead.biz/DrawBox

https://lostsidedead.biz/MouseSkybox

https://lostsidedead.biz/BoxOfRain

https://lostsidedead.biz/Rain

https://lostsidedead.biz/3DWalk

https://lostsidedead.biz/Nova

https://lostsidedead.biz/Fire

https://lostsidedead.biz/Winter

https://lostsidedead.biz/SpaceExpanse


## Usage

Include the `libmx2` headers in your project and link against the compiled library.
Refer to the `examples` directory for sample usage scenarios. Also various subfolders for example projects

## 2D Examples Included

* asteroids
* blocks
* breakout
* frogger
* knights tour
* mad
* matrix
* piece
* pong
* space shooter

## OpenGL Examples

* gl_window — Basic GL Window with Sprite
* gl_example — Hello Triangle
* gl_skeleton — OpenGL Skeleton Template
* gl_es3 — OpenGL ES 3.0 Skeleton
* gl_cube — Lit 3D Cube
* gl_animated_cube — Animated Textured Cube
* gl_cubemap — Cubemap Textured Cube
* gl_texture_map — Texture Mapping with Distortion Shaders
* gl_pyramid — Textured 3D Pyramid
* gl_pong — 3D Pong Game
* gl_breakout — Breakout Game ( https://lostsidedead.biz/game/3DBreak )
* gl_asteroids — Asteroids 3D Game ( https://lostsidedead.biz/Asteroids3D )
* gl_ship — Star Fighter Space Game
* gl_mxmod — MX Model Viewer
* gl_blank_model — Blank Model Viewer Template
* gl_object — Spinning Kaleidoscope Model
* gl_warp_model — Model Vertex Warping
* gl_about — Interactive Shader Gallery
* gl_animation — Shader Animation Player
* gl_blend_shader — Shader Blend Demo
* gl_intro_screen — 3D Intro Splash Screen
* gl_shade1 — Phong Shading (Saturn)
* gl_shade2 — Phong Shading (Torus)
* gl_shadow — Shadow Mapping Demo ( https://lostsidedead.biz/Shadow )
* gl_stencil_test — Stencil Buffer Test
* gl_framebuffer — Framebuffer Screenshot Demo
* gl_surface_test — SDL Surface to GL Texture Test
* gl_fog — Fog Effect Room
* gl_curve — Interactive Bezier Curve Editor
* gl_console — Console Overlay Demo
* gl_console_full — Console with Shader Effects
* gl_console_skeleton — Console Skeleton Template
* gl_matrix — Matrix Rain 3D
* gl_matrix_glitch — Matrix Glitch Model
* gl_matrix_psyche — Psychedelic Matrix Model
* gl_matrix_room — Matrix Room Scene
* gl_glitch — 3D Model Glitch Effect
* gl_glitch_cube — Glitch Cube v1
* gl_glitch_cube2 — Glitch Cube v2
* gl_glitch_cube3 — Glitch Cube v3 (Vertex Warp)
* gl_glitch_ex — Model Glitch + Explosion
* gl_model_glitch — Model Glitch + Particle Burst
* gl_model_explode — Model Particle Explosion
* gl_obj_explode — Planet Explosion Demo
* gl_explode — Particle Explosion Effect
* gl_ps — 2D Particle System ( https://lostsidedead.biz/point-sprites )
* gl_ps2 — Holiday Particle Effect
* gl_ps3 — Spiral Particle Effect
* gl_ps3d — 3D Particle System
* gl_ps4 — Image Dissolve Particles
* gl_fire — Fire Particle Emitter ( https://lostsidedead.biz/Fire )
* gl_rain — Rain Particle Simulation ( https://lostsidedead.biz/Rain )
* gl_snow — Snowfall Simulation
* gl_water — Water Particle Fountain
* gl_stars3d — 3D Starfield
* gl_work — Starfield with Constellations
* gl_skybox — Skybox Viewer ( https://lostsidedead.biz/Skybox )
* gl_skybox2 — Skybox Viewer (Shader Files)
* gl_skybox3 — Animated Skybox with Effects
* gl_room — 3D Room with Models
* gl_walk — First-Person Walkthrough ( https://lostsidedead.biz/3DWalk )
* gl_walktest — 3D Room Walkthrough
* space3d — Space3D: Attack of the Drones ( https://lostsidedead.biz/game/Space3D )

## Vulkan Examples

* vk_skeleton — Vulkan App Template
* vk_cube — Rotating Colored Cube
* vk_tex_cube — Textured Cube with Background
* vk_tex_pyramid — Textured Pyramid with Phong Lighting
* vk_frag — Fragment Shader Lighting Demo
* vk_data — Texture Upload Example
* vk_image — Animated Image Shader
* vk_text — Vulkan Text Rendering
* vk_sprite — 2D Sprite Batch Demo
* vk_wire — Wireframe Model Renderer
* vk_skybox — Cubemap Skybox Viewer
* vk_point_sprite — Glowing Light Orbs (Point Sprites)
* vk_stars — 3D Star Renderer
* vk_cross_stars — 3D Starfield Simulation
* vk_matrix — 3D Digital Rain
* vk_matrix_ray — Raycasting Engine with Matrix Walls
* vk_matrix_cv — Raycaster with OpenCV Webcam Textures
* vk_raycast — First-Person Raycasting Engine
* vk_fractal — Interactive Mandelbrot Explorer
* vk_pong — 3D Pong Game
* vk_spacerox — Asteroids Arcade Game
* vk_2D_MasterPiece — MasterPiece 2D Puzzle Game
* vk_mxmodel — MX Model Viewer 
* vk_borg — Textured 3D Model with Shader Effects

## How to Run the Examples

The examples require the resources for each instance to be within the same directory as the executable or to pass the path of that directory to the program with the -p or --path argument.
If you created the build directory at the repo root, to run gl_pong from `build/gl_pong/` would be:

```bash
./gl_pong -p ../../libmx/gl_pong/v4
```
or to run space from `build/space/`:

```bash
./space -p ../../libmx/space
```
If you are using Windows and MSYS/MINGW64 it's a requirement to copy the libmx.dll file into the same directory as the EXE file. From inside say the `build/gl_pong/` directory:

```bash
cp ../libmx/libmx.dll .
```

Then I would recommend running my ldd-deploy project https://github.com/lostjared/ldd-deploy/tree/main/C%2B%2B20 mingw-deploy on the EXE to copy all the required DLL Files

```bash
mingw-deploy -i gl_pong.exe -o .
```

## WebAssembly Makefiles

The project includes `Makefile.em` files for building with Emscripten. The modern Makefiles use Emscripten's built-in port system for zlib and libpng, so you **no longer need to manually compile or install** those libraries. Emscripten handles them automatically via flags:

```makefile
-s USE_ZLIB=1
-s USE_LIBPNG=1
-s USE_SDL=2
-s USE_SDL_TTF=2
-s USE_SDL_MIXER=2
-s USE_WEBGL2=1
-s FULL_ES3
```

The only thing you need to install on your host system is GLM:

```bash
sudo pacman -S glm        # Arch Linux
# or your distro's equivalent
```

### Building the Library for Emscripten

First, build and install the `libmx` static library for Emscripten. The Makefile installs headers and the archive to `$(HOME)/emscripten-libs/mx2/`:

```bash
cd libmx/libmx
make -f Makefile.em
make -f Makefile.em install
```

This places the headers in `~/emscripten-libs/mx2/include/` and `libmx.a` in `~/emscripten-libs/mx2/lib/`.

### Building an Example Project

Each example's `Makefile.em` automatically finds the installed library at `$(HOME)/emscripten-libs/mx2/`:

```makefile
LIBS_PATH = $(HOME)/emscripten-libs
MX_INCLUDE = -I$(LIBS_PATH)/mx2/include -I/usr/include/glm
LIBMX_LIB = $(LIBS_PATH)/mx2/lib/libmx.a
```

To build any example:

```bash
cd libmx/gl_breakout    # or any example directory
make -f Makefile.em
```

This produces an `.html` file along with `.wasm`, `.js`, and `.data` files (assets bundled via `--preload-file data`) that can be served from any web server.

## Playing the Example games in WebAssembly

You can point your browser to https://lostsidedead.biz/
and select Free Web Apps and the projects are available to play in your browser
  
## Contributing

Contributions are welcome!
Please fork the repository, create a new branch for your feature or bugfix, and submit a pull request.

## License

This project is licensed under the MIT License.
See the [LICENSE](LICENSE) file for details.

## Acknowledgments

Special thanks to the SDL2 community for providing a robust multimedia library.

Screenshots of Examples

![af_Console](https://github.com/user-attachments/assets/0a74a012-f22d-4e5b-94b7-9c1a72a7590c)
![libmx matrix](https://github.com/user-attachments/assets/df69b3c1-1d1f-4806-9366-21a480494461)
![vb_tetris](https://github.com/user-attachments/assets/efbb3881-ba0a-483c-92db-4abba66c61d8)
![libmx knight 2](https://github.com/user-attachments/assets/0f12d584-6478-4df7-8da0-81903ce7eac6)
![vb_pong](https://github.com/user-attachments/assets/ae63f64b-1830-4a2d-a3ed-26ccc6fd2578)

