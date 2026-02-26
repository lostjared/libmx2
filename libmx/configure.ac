dnl ============================================================================
dnl  libmx2 – Autotools build system
dnl  Mirrors the CMake build (libmx2/libmx/CMakeLists.txt)
dnl ============================================================================

AC_PREREQ([2.69])
AC_INIT([libmx2], [2.0.0], [], [libmx2])
AC_CONFIG_SRCDIR([libmx/mx.cpp])
AC_CONFIG_HEADERS([config.h:config_autotools.h.in])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall dist-xz])

dnl ── Compilers & tools ──────────────────────────────────────────────────────
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_LANG([C++])

dnl ── Libtool ────────────────────────────────────────────────────────────────
LT_PREREQ([2.4])
LT_INIT([dlopen pic-only])

dnl ── Check for C++20 support ────────────────────────────────────────────────
AC_MSG_CHECKING([for C++20 support])
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++20"
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    #if __cplusplus < 202002L
    #error C++20 required
    #endif
    #include <optional>
    #include <string>
    int main() {
      std::optional<std::string> s;
      return 0;
    }
  ]])],
  [AC_MSG_RESULT([yes])
   CXX20_CXXFLAGS="-std=c++20"],
  [CXXFLAGS="$saved_CXXFLAGS -std=c++2a"
   AC_COMPILE_IFELSE(
     [AC_LANG_SOURCE([[
       #include <optional>
       int main() { std::optional<int> x; return 0; }
     ]])],
     [AC_MSG_RESULT([yes (c++2a)])
      CXX20_CXXFLAGS="-std=c++2a"],
     [AC_MSG_RESULT([no])
      AC_MSG_ERROR([A C++20-capable compiler is required. Please upgrade gcc >= 10 or clang >= 11.])]
   )]
)
CXXFLAGS="$saved_CXXFLAGS"
AC_SUBST([CXX20_CXXFLAGS])

dnl ── Required C standard headers ───────────────────────────────────────────
AC_CHECK_HEADERS([stdlib.h stdio.h stdint.h string.h], [],
  [AC_MSG_ERROR([Missing required C standard headers.])])

dnl ── Required C++ standard headers ─────────────────────────────────────────
AC_MSG_CHECKING([for required C++ standard headers])
saved_CXXFLAGS2="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $CXX20_CXXFLAGS"
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    #include <iostream>
    #include <string>
    #include <vector>
    #include <memory>
    #include <fstream>
    #include <sstream>
    #include <optional>
    #include <thread>
    #include <mutex>
    #include <atomic>
    #include <functional>
    #include <unordered_map>
    #include <queue>
    #include <condition_variable>
    #include <set>
    #include <array>
    #include <algorithm>
    #include <stdexcept>
    int main() { return 0; }
  ]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
   AC_MSG_ERROR([Missing required C++ standard headers. Ensure your compiler supports C++20.])]
)
CXXFLAGS="$saved_CXXFLAGS2"

dnl ── pkg-config ─────────────────────────────────────────────────────────────
PKG_PROG_PKG_CONFIG
if test "x$PKG_CONFIG" = "x"; then
  AC_MSG_ERROR([pkg-config is required. Install it first.])
fi

dnl ══════════════════════════════════════════════════════════════════════════
dnl  REQUIRED DEPENDENCIES
dnl ══════════════════════════════════════════════════════════════════════════

dnl ── SDL2 (required) ────────────────────────────────────────────────────────
PKG_CHECK_MODULES([SDL2], [sdl2], [
  AC_MSG_NOTICE([SDL2 found])
], [
  AC_MSG_ERROR([SDL2 is required but not found.
    Arch:          sudo pacman -S sdl2
    Debian/Ubuntu: sudo apt install libsdl2-dev
    Fedora:        sudo dnf install SDL2-devel
    macOS:         brew install sdl2])
])

dnl ── SDL2_ttf (required) ───────────────────────────────────────────────────
PKG_CHECK_MODULES([SDL2_TTF], [SDL2_ttf], [
  AC_MSG_NOTICE([SDL2_ttf found])
], [
  AC_MSG_ERROR([SDL2_ttf is required but not found.
    Arch:          sudo pacman -S sdl2_ttf
    Debian/Ubuntu: sudo apt install libsdl2-ttf-dev
    Fedora:        sudo dnf install SDL2_ttf-devel
    macOS:         brew install sdl2_ttf])
])

dnl ── zlib (required) ───────────────────────────────────────────────────────
PKG_CHECK_MODULES([ZLIB], [zlib], [
  AC_MSG_NOTICE([ZLIB found])
], [
  AC_MSG_ERROR([zlib is required but not found.
    Arch:          sudo pacman -S zlib
    Debian/Ubuntu: sudo apt install zlib1g-dev
    Fedora:        sudo dnf install zlib-devel
    macOS:         brew install zlib])
])
AC_CHECK_HEADER([zlib.h], [], [AC_MSG_ERROR([zlib.h header not found.])])

dnl ── libpng (required) ─────────────────────────────────────────────────────
PKG_CHECK_MODULES([PNG], [libpng], [
  AC_MSG_NOTICE([libpng found])
], [
  PKG_CHECK_MODULES([PNG], [libpng16], [
    AC_MSG_NOTICE([libpng16 found])
  ], [
    AC_MSG_ERROR([libpng is required but not found.
      Arch:          sudo pacman -S libpng
      Debian/Ubuntu: sudo apt install libpng-dev
      Fedora:        sudo dnf install libpng-devel
      macOS:         brew install libpng])
  ])
])
AC_CHECK_HEADER([png.h], [], [AC_MSG_ERROR([png.h header not found.])])

dnl ── pthreads ──────────────────────────────────────────────────────────────
AC_CHECK_LIB([pthread], [pthread_create], [PTHREAD_LIBS="-lpthread"], [PTHREAD_LIBS=""])
AC_SUBST([PTHREAD_LIBS])

dnl ── dlopen (for volk / Vulkan loader) ─────────────────────────────────────
DL_LIBS=""
AC_CHECK_FUNC([dlopen], [], [
  AC_CHECK_LIB([dl], [dlopen], [DL_LIBS="-ldl"], [])
])
AC_SUBST([DL_LIBS])

dnl ══════════════════════════════════════════════════════════════════════════
dnl  OPTIONAL FEATURES
dnl ══════════════════════════════════════════════════════════════════════════

dnl ── Debug mode ─────────────────────────────────────────────────────────────
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug], [Build with debug symbols @<:@default=no@:>@])],
  [enable_debug=$enableval], [enable_debug=no])

if test "x$enable_debug" = "xyes"; then
  AC_DEFINE([DEBUG_MODE], [1], [Define to 1 for debug mode])
  OPT_CFLAGS="-g -O0"
  OPT_CXXFLAGS="-g -O0"
else
  OPT_CFLAGS="-O3"
  OPT_CXXFLAGS="-O3"
fi
AC_SUBST([OPT_CFLAGS])
AC_SUBST([OPT_CXXFLAGS])

dnl ── SDL2_mixer (sound) ────────────────────────────────────────────────────
AC_ARG_ENABLE([mixer],
  [AS_HELP_STRING([--enable-mixer], [Build with SDL2_mixer sound support @<:@default=yes@:>@])],
  [enable_mixer=$enableval], [enable_mixer=yes])

if test "x$enable_mixer" = "xyes"; then
  PKG_CHECK_MODULES([SDL2_MIXER], [SDL2_mixer], [
    AC_DEFINE([WITH_MIXER], [1], [Define to 1 if SDL2_mixer is available])
    AC_MSG_NOTICE([SDL2_mixer found – mixer support enabled])
  ], [
    AC_MSG_WARN([SDL2_mixer not found – disabling mixer support.
      Arch:          sudo pacman -S sdl2_mixer
      Debian/Ubuntu: sudo apt install libsdl2-mixer-dev])
    enable_mixer=no
  ])
fi
AM_CONDITIONAL([ENABLE_MIXER], [test "x$enable_mixer" = "xyes"])

dnl ── JPEG ──────────────────────────────────────────────────────────────────
AC_ARG_ENABLE([jpeg],
  [AS_HELP_STRING([--enable-jpeg], [Build with JPEG support @<:@default=no@:>@])],
  [enable_jpeg=$enableval], [enable_jpeg=no])

if test "x$enable_jpeg" = "xyes"; then
  PKG_CHECK_MODULES([JPEG], [libjpeg], [have_jpeg=yes], [
    AC_CHECK_LIB([jpeg], [jpeg_read_header], [
      JPEG_LIBS="-ljpeg"
      JPEG_CFLAGS=""
      have_jpeg=yes
    ], [have_jpeg=no])
  ])
  if test "x$have_jpeg" = "xyes"; then
    AC_CHECK_HEADER([jpeglib.h], [], [
      AC_MSG_WARN([jpeglib.h not found])
      have_jpeg=no
    ])
  fi
  if test "x$have_jpeg" = "xyes"; then
    AC_DEFINE([WITH_JPEG], [1], [Define to 1 if JPEG support is available])
    AC_MSG_NOTICE([JPEG support enabled])
  else
    AC_MSG_WARN([JPEG not found – disabling JPEG support.
      Arch:          sudo pacman -S libjpeg-turbo
      Debian/Ubuntu: sudo apt install libjpeg-dev])
    enable_jpeg=no
  fi
fi
AM_CONDITIONAL([ENABLE_JPEG], [test "x$enable_jpeg" = "xyes"])

dnl ── OpenGL ────────────────────────────────────────────────────────────────
AC_ARG_ENABLE([opengl],
  [AS_HELP_STRING([--enable-opengl], [Build with OpenGL support @<:@default=yes@:>@])],
  [enable_opengl=$enableval], [enable_opengl=yes])

GL_DEFINES=""
if test "x$enable_opengl" = "xyes"; then
  have_gl=no
  PKG_CHECK_MODULES([GL], [gl], [have_gl=yes], [
    AC_CHECK_LIB([GL], [glClear], [
      GL_LIBS="-lGL"
      GL_CFLAGS=""
      have_gl=yes
    ], [
      AC_CHECK_LIB([OpenGL], [glClear], [
        GL_LIBS="-lOpenGL"
        GL_CFLAGS=""
        have_gl=yes
      ], [have_gl=no])
    ])
  ])
  if test "x$have_gl" = "xyes"; then
    dnl Check for GLM (header-only)
    have_glm=no
    PKG_CHECK_MODULES([GLM], [glm], [have_glm=yes], [
      AC_CHECK_HEADER([glm/glm.hpp], [have_glm=yes], [have_glm=no])
    ])
    if test "x$have_glm" = "xyes"; then
      AC_DEFINE([WITH_GL], [1], [Define to 1 if OpenGL support is enabled])
      GL_DEFINES="-DWITH_GL"
      AC_MSG_NOTICE([OpenGL + GLM support enabled])
    else
      AC_MSG_WARN([GLM not found – disabling OpenGL.
        Arch:          sudo pacman -S glm
        Debian/Ubuntu: sudo apt install libglm-dev])
      enable_opengl=no
    fi
  else
    AC_MSG_WARN([OpenGL not found – disabling OpenGL support.
      Arch:          sudo pacman -S mesa
      Debian/Ubuntu: sudo apt install libgl-dev])
    enable_opengl=no
  fi
fi
AC_SUBST([GL_DEFINES])
AC_SUBST([GL_LIBS])
AC_SUBST([GL_CFLAGS])
AC_SUBST([GLM_CFLAGS])
AM_CONDITIONAL([ENABLE_OPENGL], [test "x$enable_opengl" = "xyes"])

dnl ── OpenGL ES3 ────────────────────────────────────────────────────────────
AC_ARG_ENABLE([es3],
  [AS_HELP_STRING([--enable-es3], [Build with OpenGL ES3 support @<:@default=no@:>@])],
  [enable_es3=$enableval], [enable_es3=no])

if test "x$enable_es3" = "xyes"; then
  AC_CHECK_HEADER([GLES3/gl3.h], [
    AC_DEFINE([WITH_OPENGL_ES3], [1], [Define to 1 if OpenGL ES3 is available])
    AC_MSG_NOTICE([OpenGL ES3 support enabled])
  ], [
    AC_MSG_WARN([GLES3/gl3.h not found – disabling ES3.
      Arch:          sudo pacman -S mesa
      Debian/Ubuntu: sudo apt install libgles2-mesa-dev])
    enable_es3=no
  ])
fi
AM_CONDITIONAL([ENABLE_ES3], [test "x$enable_es3" = "xyes"])

dnl ── Vulkan ────────────────────────────────────────────────────────────────
AC_ARG_ENABLE([vulkan],
  [AS_HELP_STRING([--enable-vulkan], [Build with Vulkan support @<:@default=no@:>@])],
  [enable_vulkan=$enableval], [enable_vulkan=no])

VK_DEFINES=""
if test "x$enable_vulkan" = "xyes"; then
  have_vulkan=no
  PKG_CHECK_MODULES([VULKAN], [vulkan], [have_vulkan=yes], [
    AC_CHECK_HEADER([vulkan/vulkan.h], [
      AC_CHECK_LIB([vulkan], [vkCreateInstance], [
        VULKAN_LIBS="-lvulkan"
        VULKAN_CFLAGS=""
        have_vulkan=yes
      ], [have_vulkan=no])
    ], [have_vulkan=no])
  ])

  if test "x$have_vulkan" = "xyes"; then
    dnl GLM is also required for Vulkan
    have_glm=no
    PKG_CHECK_MODULES([GLM], [glm], [have_glm=yes], [
      AC_CHECK_HEADER([glm/glm.hpp], [have_glm=yes], [have_glm=no])
    ])
    if test "x$have_glm" = "xyes"; then
      AC_DEFINE([WITH_VK], [1], [Define to 1 if Vulkan support is enabled])
      VK_DEFINES="-DWITH_VK"
      AC_MSG_NOTICE([Vulkan support enabled])
    else
      AC_MSG_WARN([GLM not found for Vulkan – disabling.
        Arch:          sudo pacman -S glm
        Debian/Ubuntu: sudo apt install libglm-dev])
      enable_vulkan=no
    fi
  else
    AC_MSG_WARN([Vulkan not found – disabling Vulkan support.
      Arch:          sudo pacman -S vulkan-headers vulkan-icd-loader
      Debian/Ubuntu: sudo apt install libvulkan-dev
      Fedora:        sudo dnf install vulkan-devel])
    enable_vulkan=no
  fi
fi
AC_SUBST([VK_DEFINES])
AC_SUBST([VULKAN_LIBS])
AC_SUBST([VULKAN_CFLAGS])
AM_CONDITIONAL([ENABLE_VULKAN], [test "x$enable_vulkan" = "xyes"])

dnl ── OpenCV ────────────────────────────────────────────────────────────────
AC_ARG_ENABLE([opencv],
  [AS_HELP_STRING([--enable-opencv], [Build with OpenCV support @<:@default=no@:>@])],
  [enable_opencv=$enableval], [enable_opencv=no])

OPENCV_PC_MODULE=""
if test "x$enable_opencv" = "xyes"; then
  have_opencv=no
  PKG_CHECK_MODULES([OPENCV], [opencv4], [have_opencv=yes; OPENCV_PC_MODULE=opencv4], [
    PKG_CHECK_MODULES([OPENCV], [opencv], [have_opencv=yes; OPENCV_PC_MODULE=opencv], [have_opencv=no])
  ])
  if test "x$have_opencv" = "xyes"; then
    AC_DEFINE([WITH_OPENCV], [1], [Define to 1 if OpenCV is available])
    AC_MSG_NOTICE([OpenCV support enabled (module: $OPENCV_PC_MODULE)])
  else
    AC_MSG_WARN([OpenCV not found – disabling OpenCV support.
      Arch:          sudo pacman -S opencv
      Debian/Ubuntu: sudo apt install libopencv-dev])
    enable_opencv=no
  fi
fi
AC_SUBST([OPENCV_CFLAGS])
AC_SUBST([OPENCV_LIBS])
AC_SUBST([OPENCV_PC_MODULE])
AM_CONDITIONAL([ENABLE_OPENCV], [test "x$enable_opencv" = "xyes"])

dnl ── Examples ──────────────────────────────────────────────────────────────
AC_ARG_ENABLE([examples],
  [AS_HELP_STRING([--enable-examples], [Build example programs @<:@default=yes@:>@])],
  [enable_examples=$enableval], [enable_examples=yes])
AM_CONDITIONAL([ENABLE_EXAMPLES], [test "x$enable_examples" = "xyes"])

dnl ── Cross GL/VK examples ─────────────────────────────────────────────────
AC_ARG_ENABLE([cross],
  [AS_HELP_STRING([--enable-cross], [Build cross GL/VK example programs @<:@default=no@:>@])],
  [enable_cross=$enableval], [enable_cross=no])

if test "x$enable_cross" = "xyes"; then
  if test "x$enable_opengl" != "xyes" && test "x$enable_vulkan" != "xyes"; then
    AC_MSG_WARN([--enable-cross requires --enable-opengl or --enable-vulkan – disabling.])
    enable_cross=no
  fi
fi
AM_CONDITIONAL([ENABLE_CROSS], [test "x$enable_cross" = "xyes"])
AM_CONDITIONAL([CROSS_USE_VK], [test "x$enable_cross" = "xyes" -a "x$enable_vulkan" = "xyes"])
AM_CONDITIONAL([CROSS_USE_GL], [test "x$enable_cross" = "xyes" -a "x$enable_vulkan" != "xyes" -a "x$enable_opengl" = "xyes"])

dnl ── Version defines for config.h ──────────────────────────────────────────
AC_DEFINE_UNQUOTED([PROJECT_VERSION_MAJOR], [2], [Major version number])
AC_DEFINE_UNQUOTED([PROJECT_VERSION_MINOR], [0], [Minor version number])

dnl ── Output ────────────────────────────────────────────────────────────────
AC_CONFIG_FILES([Makefile
                 libmx.pc
                 libmxgl.pc
                 libmxvk.pc
                 libmxgl_cv.pc
                 libmxvk_cv.pc])
AC_OUTPUT

dnl ══════════════════════════════════════════════════════════════════════════
dnl  Summary
dnl ══════════════════════════════════════════════════════════════════════════
AC_MSG_NOTICE([])
AC_MSG_NOTICE([=== libmx2 $PACKAGE_VERSION Configuration Summary ===])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([  Prefix:         $prefix])
AC_MSG_NOTICE([  C++ compiler:   $CXX])
AC_MSG_NOTICE([  C++ standard:   C++20 ($CXX20_CXXFLAGS)])
AC_MSG_NOTICE([  Build type:     $(if test "x$enable_debug" = "xyes"; then echo Debug; else echo Release; fi)])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([  Required dependencies:])
AC_MSG_NOTICE([    SDL2:          yes])
AC_MSG_NOTICE([    SDL2_ttf:      yes])
AC_MSG_NOTICE([    zlib:          yes])
AC_MSG_NOTICE([    libpng:        yes])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([  Optional features:])
AC_MSG_NOTICE([    SDL2_mixer:    $enable_mixer])
AC_MSG_NOTICE([    JPEG:          $enable_jpeg])
AC_MSG_NOTICE([    OpenGL:        $enable_opengl])
AC_MSG_NOTICE([    OpenGL ES3:    $enable_es3])
AC_MSG_NOTICE([    Vulkan:        $enable_vulkan])
AC_MSG_NOTICE([    OpenCV:        $enable_opencv])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([  Build options:])
AC_MSG_NOTICE([    Examples:      $enable_examples])
AC_MSG_NOTICE([    Cross GL/VK:   $enable_cross])
AC_MSG_NOTICE([])
