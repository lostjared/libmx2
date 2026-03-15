/**
 * @file config.hpp
 * @brief Compile-time feature flags and version macros for libmx.
 *
 * Defines feature-enable macros (WITH_GL, WITH_MIXER) and the project
 * version numbers when not already specified by the build system.
 */
#ifndef __FILES__H_
#define __FILES__H_

/** @brief Enable OpenGL support. */
#ifndef WITH_GL
#define WITH_GL
#endif

/** @brief Enable SDL_mixer audio support. */
#ifndef WITH_MIXER
#define WITH_MIXER
#endif

/** @brief Major version number of the project. */
#ifndef PROJECT_VERSION_MAJOR
#define PROJECT_VERSION_MAJOR 1
#endif

/** @brief Minor version number of the project. */
#ifndef PROJECT_VERSION_MINOR
#define PROJECT_VERSION_MINOR 0
#endif
#endif
