
# libmx2

![image](https://github.com/user-attachments/assets/70299ae9-2d60-4a9a-8327-390145eb9191)

libmx2 is a cross-platform library designed to facilitate SDL2 development using C++20.
It provides a collection of utilities and abstractions to streamline the creation of multimedia applications.

## Motivation

The motivation for creating this library was to give me an easy way to put together new SDL2 applications 
while using an approach I prefer which is an Object Oriented Design.

## Features

- **SDL2 Integration**: Simplifies the setup and management of SDL2 components.
- **Modern C++20 Design**: Utilizes C++20 features for cleaner and more efficient code.
- **Modular Architecture**: Offers a flexible structure to accommodate various project needs.
- **Optional OpenGL/GLEW/GLM support**: You can compile in support for these libraries with examples
## Getting Started

### Prerequisites

- C++20 compatible compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+
- SDL2/SDL2_ttf, libpng, zlib installed
- Optional: OpenGL/GLEW/GLM

### Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/lostjared/libmx2.git
   ```

2. Navigate to the project directory:

   ```bash
   cd libmx2
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

## Usage

Include the `libmx2` headers in your project and link against the compiled library.
Refer to the `examples` directory for sample usage scenarios. Also various subfolders for example projects

## Examples Included

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
  

## Contributing

Contributions are welcome!
Please fork the repository, create a new branch for your feature or bugfix, and submit a pull request.

## License

This project is licensed under the MIT License.
See the [LICENSE](LICENSE) file for details.

## Acknowledgments

Special thanks to the SDL2 community for providing a robust multimedia library.

Screenshots of Examples

![vb_tetris](https://github.com/user-attachments/assets/efbb3881-ba0a-483c-92db-4abba66c61d8)
![vb_aster](https://github.com/user-attachments/assets/91bf11ad-b171-4363-96be-cad2bd7f5ea2)
![vb_pong](https://github.com/user-attachments/assets/ae63f64b-1830-4a2d-a3ed-26ccc6fd2578)

