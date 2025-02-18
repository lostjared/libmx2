# libmx2

![libmx cube](https://github.com/user-attachments/assets/68a4a55e-98a3-4bcc-9cf6-f6bf3cbb7b86)
![breakout-3d](https://github.com/user-attachments/assets/bd0a7d5d-fdfb-4118-aaa5-6b5975129fd5)

libmx2 is a cross-platform library that facilitates cross-platform SDL2/OpenGL development using C++20.
It provides a collection of utilities and abstractions to streamline the creation of multimedia applications.

## Motivation

The motivation for creating this library was to give me an easy way to assemble new SDL2 applications 
while using an approach I prefer, an Object Oriented Design. I have experimented with AI in some of the example projects for this library.

# In Progress
I am working on adding support for Vulkan

## Features

- **SDL2 Integration**: Simplifies the setup and management of SDL2 components.
- **Modern C++20 Design**: Utilizes C++20 features for cleaner and more efficient code.
- **Modular Architecture**: Offers a flexible structure to accommodate various project needs.
- **Optional OpenGL/GLAD/GLM support**: You can compile in support for these libraries with examples
- **Optional Vulkan**: You can compile in support for Vulkan or MoltenVK
## Getting Started

### Prerequisites

- C++20 compatible compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+
- SDL2/SDL2_ttf/SDL2_mixer, libpng, zlib installed
- Optional: OpenGL/GLM includes GLAD or Vulkan/MoletnVK

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/lostjared/libmx2.git
   ```

2. Navigate to the project directory:

   ```bash
   cd libmx2/libmx
   ```

3. Build the library using CMake:

   ```bash
   mkdir build
   cd build
   cmake ..
   make -j4
   sudo make install
   ```

Optionally you can build as a static library


   ```bash
   mkdir build
   cd build
   cmake .. -DBUILD_STATIC_LIB=ON
   make -j4
   sudo make install
   ```
Current Options:

* -DJPEG=OFF - optionally compile in support for JPEG images with jpeglib ON or OFF
* -DBUILD_STATIC_LIB=OFF - optionally compile library as a static library ON or OFF
* -DOPENGL=ON optionally compile OpenGL support into the library either ON or OFF
* -DMIXER=ON optionally compile audio support with SDL2_mixer either ON or OFF
* -DVULKAN=ON optionally compile Vulkan support into the library ON  or OFF
* -DMOLTEN=ON optionally compile with MoltenVK
* =DMOLTEN_PATH=/usr/local/opt/molten-vk  (or path you installed molten-vk)


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
* Basic GL Window
* Triangle
* Skeleton (Hello, World!)
* 3D Texture Mapped Cube
* 3D Texture Mapped Animated Cube
* 3D Pyramid with Lighting
* GL Pong with Lighting
* 3D Breakout ( https://lostsidedead.biz/game/3DBreak )
* MXMOD Format/3D Model Viewer
* Space3D: Attack of the Drones ( https://lostsidedead.biz/game/Space3D )
* Four Point Sprite Examples ( gl_ps, gl_ps2, gl_ps3, gl_ps4 https://lostsidedead.biz/point-sprites )
* 3D Snow Particle Demo ( https://lostsidedead.biz/holiday/3d.html )
* 
## How to Run the Examples

The examples require the resources for each instance to be within the same directory as the executable or to pass the path of that directory to the program with the -p or --path argument.
If you created the build directory an example to run gl_pong from build/gl_pong would be

```bash
./gl_pong -p ../../gl_pong/v4
```
or to run space:

```bash
./space -p ../../space
```
If you are using Windows and MSYS/MINGW64 its a requirement to copy the libmx.dll file into the same directory as the EXE file. From inside say the build/gl_pong directory

```bash
cp ../libmx/libmx.dll .
```

Then I would recommend running my ldd-deploy project https://github.com/lostjared/ldd-deploy/tree/main/C%2B%2B20 mingw-deploy on the EXE to copy all the required DLL Files

```bash
mingw-deploy -i gl_pong.exe -o .
```

## WebAssembly Makefiles

To compile the WebAssembly makefiles is a little more difficult you will first need to create a folder for libpng16 zlib and libmx to be installed to you will need to edit the Makefile and place
the path to this folder

```
LIBS_PATH = /home/jared/emscripten-libs
```

then install libpng16, zlib by compiling them with emscripten and placing the files in this location
you will also need to install GLM on your host system use  your package manager to get it  something like
```
sudo pacman -S glm
```

the paths in the Makefile look like this

```
ZLIB_INCLUDE = -I$(LIBS_PATH)/zlib/include
PNG_INCLUDE = -I$(LIBS_PATH)/libpng/include
MX_INCLUDE = -I$(LIBS_PATH)/mx2/include -I/usr/include/glm
ZLIB_LIB = $(LIBS_PATH)/zlib/lib/libz.a
PNG_LIB = $(LIBS_PATH)/libpng/lib/libpng.a
```

then enter the libmx directory edit the Makefile for the path
and

```bash
make -f Makefile.em
sudo make -f Makefile.em install
```

then you should be able to build the projects with emscripten after setting the path in each Makefile.em using

```bash
make -f Makefile.em
```

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

![libmx matrix](https://github.com/user-attachments/assets/df69b3c1-1d1f-4806-9366-21a480494461)
![vb_tetris](https://github.com/user-attachments/assets/efbb3881-ba0a-483c-92db-4abba66c61d8)
![libmx knight 2](https://github.com/user-attachments/assets/0f12d584-6478-4df7-8da0-81903ce7eac6)
![vb_aster](https://github.com/user-attachments/assets/91bf11ad-b171-4363-96be-cad2bd7f5ea2)
![vb_pong](https://github.com/user-attachments/assets/ae63f64b-1830-4a2d-a3ed-26ccc6fd2578)

