## ============================================================================
##  libmx2 – top-level Makefile.am  (non-recursive, uses subdir-objects)
##
##  Binary names use hyphens to avoid colliding with source subdirectories.
##  Automake canonicalises hyphens → underscores for variable prefixes, so
##      mx2-gl-example  ↔  mx2_gl_example_SOURCES / mx2_gl_example_LDADD
## ============================================================================

ACLOCAL_AMFLAGS = -I m4

## ---------------------------------------------------------------------------
##  Global compiler flags
## ---------------------------------------------------------------------------

AM_CPPFLAGS = \
	-I$(srcdir)/libmx \
	-I$(srcdir)/libmx/include \
	-I$(srcdir)/volk \
	-I$(builddir) \
	$(SDL2_CFLAGS) \
	$(SDL2_TTF_CFLAGS) \
	$(PNG_CFLAGS) \
	$(ZLIB_CFLAGS) \
	$(SDL2_MIXER_CFLAGS) \
	$(GL_CFLAGS) \
	$(GLM_CFLAGS) \
	$(VULKAN_CFLAGS) \
	$(JPEG_CFLAGS) \
	$(OPENCV_CFLAGS) \
	$(GL_DEFINES) \
	$(VK_DEFINES)

AM_CXXFLAGS = $(CXX20_CXXFLAGS) $(OPT_CXXFLAGS) -Wall -fPIC
AM_CFLAGS   = $(OPT_CFLAGS) -Wall -fPIC
LIBS       += $(SDL2_LIBS) $(SDL2_TTF_LIBS) $(SDL2_MIXER_LIBS)

## ---------------------------------------------------------------------------
##  Primary lists  (filled conditionally below)
## ---------------------------------------------------------------------------

lib_LTLIBRARIES   =
noinst_PROGRAMS   =
bin_PROGRAMS      =
EXTRA_PROGRAMS    =

## Header install directory: $(includedir)/mx2
mx2includedir     = $(includedir)/mx2
mx2gladdir        = $(includedir)/mx2/glad
mx2khrdir         = $(includedir)/mx2/KHR
mx2volkdir        = $(includedir)/mx2/volk

## pkg-config .pc files
pkgconfigdir      = $(libdir)/pkgconfig
pkgconfig_DATA    = libmx.pc

if ENABLE_OPENGL
pkgconfig_DATA   += libmxgl.pc
endif

if ENABLE_VULKAN
pkgconfig_DATA   += libmxvk.pc
endif

if ENABLE_OPENGL
if ENABLE_OPENCV
pkgconfig_DATA   += libmxgl_cv.pc
endif
endif

if ENABLE_VULKAN
if ENABLE_OPENCV
pkgconfig_DATA   += libmxvk_cv.pc
endif
endif

EXTRA_DIST = \
	libmx.pc.in \
	libmxgl.pc.in \
	libmxvk.pc.in \
	libmxgl_cv.pc.in \
	libmxvk_cv.pc.in

## ============================================================================
##  LIBRARIES
## ============================================================================

## ── libmx  (base SDL2 library) ─────────────────────────────────────────────

lib_LTLIBRARIES += libmx.la

libmx_la_SOURCES = \
	libmx/mx.cpp \
	libmx/util.cpp \
	libmx/loadpng.cpp \
	libmx/font.cpp \
	libmx/cfg.cpp \
	libmx/texture.cpp \
	libmx/exception.cpp \
	libmx/joystick.cpp \
	libmx/jpeg.cpp \
	libmx/input.cpp \
	libmx/sound.cpp \
	libmx/tee_stream.cpp

libmx_la_LIBADD = \
	$(SDL2_LIBS) \
	$(SDL2_TTF_LIBS) \
	$(PNG_LIBS) \
	$(ZLIB_LIBS) \
	$(PTHREAD_LIBS)

if ENABLE_MIXER
libmx_la_LIBADD += $(SDL2_MIXER_LIBS)
endif
if ENABLE_JPEG
libmx_la_LIBADD += $(JPEG_LIBS)
endif

libmx_la_LDFLAGS = -version-info 1:14:0 -no-undefined

## Installed base headers
mx2include_HEADERS = \
	libmx/mx.hpp \
	libmx/argz.hpp \
	libmx/util.hpp \
	libmx/jpeg.hpp \
	libmx/loadpng.hpp \
	libmx/object.hpp \
	libmx/font.hpp \
	libmx/cfg.hpp \
	libmx/config.hpp \
	libmx/tee_stream.hpp \
	libmx/texture.hpp \
	libmx/exception.hpp \
	libmx/joystick.hpp \
	libmx/wrapper.hpp \
	libmx/input.hpp \
	libmx/sound.hpp

## ── libmxgl  (OpenGL library) ──────────────────────────────────────────────

if ENABLE_OPENGL

lib_LTLIBRARIES += libmxgl.la

libmxgl_la_SOURCES = \
	libmx/gl.cpp \
	libmx/model.cpp \
	libmx/console.cpp \
	libmx/src/glad.c

libmxgl_la_CPPFLAGS = $(AM_CPPFLAGS) -DWITH_GL
libmxgl_la_CXXFLAGS = $(AM_CXXFLAGS)
libmxgl_la_CFLAGS   = $(AM_CFLAGS)

libmxgl_la_LIBADD = \
	libmx.la \
	$(GL_LIBS)

libmxgl_la_LDFLAGS = -version-info 1:14:0 -no-undefined

## Installed GL headers
mx2include_HEADERS += \
	libmx/gl.hpp \
	libmx/model.hpp \
	libmx/console.hpp

mx2glad_HEADERS = libmx/include/glad/glad.h
mx2khr_HEADERS  = libmx/include/KHR/khrplatform.h

endif ## ENABLE_OPENGL

## ── libvolk  (Vulkan meta-loader, static) ──────────────────────────────────

if ENABLE_VULKAN

lib_LTLIBRARIES += libvolk.la

libvolk_la_SOURCES  = volk/volk.cpp
libvolk_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/volk -DWITH_VK
libvolk_la_CXXFLAGS = $(AM_CXXFLAGS)

libvolk_la_LIBADD  = $(DL_LIBS)
libvolk_la_LDFLAGS = -static

mx2volk_HEADERS = volk/volk.h

endif ## ENABLE_VULKAN

## ── libmxvk  (Vulkan library, static) ──────────────────────────────────────

if ENABLE_VULKAN

lib_LTLIBRARIES += libmxvk.la

libmxvk_la_SOURCES = \
	libmx/vk.cpp \
	libmx/vk_model.cpp \
	libmx/vk_abstract_model.cpp \
	libmx/vk_text.cpp \
	libmx/vk_sprite.cpp

libmxvk_la_CPPFLAGS = $(AM_CPPFLAGS) -DWITH_VK
libmxvk_la_CXXFLAGS = $(AM_CXXFLAGS)

libmxvk_la_LIBADD = \
	libmx.la \
	libvolk.la \
	$(VULKAN_LIBS)

libmxvk_la_LDFLAGS = -static

## Installed VK headers
mx2include_HEADERS += \
	libmx/vk.hpp \
	libmx/vk_model.hpp \
	libmx/vk_abstract_model.hpp \
	libmx/vk_text.hpp \
	libmx/vk_sprite.hpp

endif ## ENABLE_VULKAN

## ── libmxgl_cv  (OpenGL + OpenCV bridge) ───────────────────────────────────

if ENABLE_OPENGL
if ENABLE_OPENCV

lib_LTLIBRARIES += libmxgl_cv.la

libmxgl_cv_la_SOURCES  = libmx/gl_cv.cpp
libmxgl_cv_la_CPPFLAGS = $(AM_CPPFLAGS) -DWITH_GL
libmxgl_cv_la_CXXFLAGS = $(AM_CXXFLAGS)

libmxgl_cv_la_LIBADD = \
	libmxgl.la \
	$(OPENCV_LIBS)

libmxgl_cv_la_LDFLAGS = -version-info 1:14:0 -no-undefined

mx2include_HEADERS += libmx/gl_cv.hpp

endif ## ENABLE_OPENCV
endif ## ENABLE_OPENGL

## ── libmxvk_cv  (Vulkan + OpenCV bridge) ──────────────────────────────────

if ENABLE_VULKAN
if ENABLE_OPENCV

lib_LTLIBRARIES += libmxvk_cv.la

libmxvk_cv_la_SOURCES  = libmx/vk_cv.cpp
libmxvk_cv_la_CPPFLAGS = $(AM_CPPFLAGS) -DWITH_VK
libmxvk_cv_la_CXXFLAGS = $(AM_CXXFLAGS)

libmxvk_cv_la_LIBADD = \
	libmxvk.la \
	$(OPENCV_LIBS)

libmxvk_cv_la_LDFLAGS = -version-info 1:14:0 -no-undefined

mx2include_HEADERS += libmx/vk_cv.hpp

endif ## ENABLE_OPENCV
endif ## ENABLE_VULKAN

## ============================================================================
##  UTILITY PROGRAMS  (always built & installed)
## ============================================================================

bin_PROGRAMS += mxmod-compress

mxmod_compress_SOURCES  = mxmod_compress/cmp.cpp
mxmod_compress_CPPFLAGS = -I$(srcdir)/libmx -I$(builddir) $(ZLIB_CFLAGS)
mxmod_compress_LDADD    = $(ZLIB_LIBS)

## ============================================================================
##  EXAMPLE PROGRAMS
## ============================================================================
##
##  Binary names are prefixed with "mx2-" and use hyphens so they never
##  collide with their same-named source subdirectories.
## ============================================================================

EXTRA_PROGRAMS += \
	mx2-test mx2-controller-test mx2-input-test mx2-image-test \
	mx2-knights-tour mx2-pong mx2-asteroids mx2-blocks mx2-piece mx2-matrix \
	mx2-breakout mx2-frogger mx2-mad mx2-space \
	mx2-sound-test mx2-jpeg-test mx2-example \
	mx2-gl-window mx2-gl-skeleton mx2-gl-example mx2-gl-cube mx2-gl-pong \
	mx2-gl-animated-cube mx2-gl-breakout mx2-gl-pyramid mx2-gl-mxmod \
	mx2-space3d mx2-gl-surface-test \
	mx2-gl-ps mx2-gl-ps2 mx2-gl-ps3 mx2-gl-ps4 mx2-gl-ps3d \
	mx2-gl-stars3d mx2-gl-work mx2-gl-explode mx2-gl-walk mx2-gl-walktest \
	mx2-gl-cubemap mx2-gl-stencil-test mx2-gl-blend-shader mx2-gl-framebuffer \
	mx2-gl-shade1 mx2-gl-shade2 mx2-gl-shadow \
	mx2-gl-skybox mx2-gl-skybox2 mx2-gl-skybox3 \
	mx2-gl-curve mx2-gl-snow mx2-gl-fire mx2-gl-water mx2-gl-rain mx2-gl-room \
	mx2-gl-matrix mx2-gl-obj-explode mx2-gl-fog mx2-gl-ship \
	mx2-gl-matrix-room mx2-gl-matrix-glitch mx2-gl-matrix-psyche \
	mx2-gl-blank-model mx2-gl-intro-screen mx2-gl-model-explode mx2-gl-model-glitch \
	mx2-gl-asteroids mx2-gl-console mx2-gl-console-skeleton mx2-gl-console-full \
	mx2-gl-object \
	mx2-gl-glitch mx2-gl-glitch-cube mx2-gl-glitch-cube2 mx2-gl-glitch-cube3 \
	mx2-gl-texture-map mx2-gl-glitch-ex \
	mx2-gl-about mx2-gl-animation mx2-gl-tux \
	mx2-gl-es3 mx2-gl-warp-model \
	mx2-gl-cv-skeleton mx2-gl-walktest-cv \
	mx2-vk-skeleton mx2-vk-example mx2-vk-tex-cube mx2-vk-tex-pyramid \
	mx2-vk-data mx2-vk-image mx2-vk-frag mx2-vk-borg mx2-vk-wire mx2-vk-text \
	mx2-vk-stars mx2-vk-pong mx2-vk-raycast mx2-vk-matrix mx2-vk-matrix-ray \
	mx2-vk-sprite mx2-vk-point-sprite mx2-vk-point-sprite3D \
	mx2-vk-2D-MasterPiece mx2-vk-spacerox mx2-vk-mxmodel mx2-vk-stickball \
	mx2-vk-abstract-model mx2-vk-cube mx2-vk-skybox mx2-vk-star3D mx2-vk-tux \
	mx2-vk-matrix-cv mx2-vk-opencv mx2-vk-opencv-moon mx2-vk-opencv-shader \
	mx2-cross mx2-moon mx2-vk-cross-stars mx2-vk-cross-stars-frag

## ── Base (SDL2-only) examples ──────────────────────────────────────────────

mx2_test_SOURCES              = test/main.cpp
mx2_test_LDADD                = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_controller_test_SOURCES   = controller_test/main.cpp
mx2_controller_test_LDADD     = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_input_test_SOURCES        = input_test/main.cpp
mx2_input_test_LDADD          = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_image_test_SOURCES        = image_test/main.cpp
mx2_image_test_LDADD          = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_knights_tour_SOURCES      = knights_tour/knight.cpp
mx2_knights_tour_LDADD        = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_pong_SOURCES              = pong/pong.cpp
mx2_pong_LDADD                = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_asteroids_SOURCES         = asteroids/asteroids.cpp asteroids/game.cpp
mx2_asteroids_LDADD           = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_blocks_SOURCES            = blocks/blocks.cpp
mx2_blocks_LDADD              = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_piece_SOURCES             = piece/piece.cpp
mx2_piece_LDADD               = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_matrix_SOURCES            = matrix/matrix.cpp
mx2_matrix_LDADD              = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_breakout_SOURCES          = breakout/breakout.cpp
mx2_breakout_LDADD            = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_frogger_SOURCES           = frogger/frogger.cpp
mx2_frogger_LDADD             = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_mad_SOURCES               = mad/mad.cpp
mx2_mad_LDADD                 = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_space_SOURCES             = space/space.cpp
mx2_space_LDADD               = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

mx2_example_SOURCES           = example/main.cpp
mx2_example_LDADD             = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

## ── Mixer example ──────────────────────────────────────────────────────────

mx2_sound_test_SOURCES        = sound_test/main.cpp
mx2_sound_test_LDADD          = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

## ── JPEG example ───────────────────────────────────────────────────────────

mx2_jpeg_test_SOURCES         = jpeg_test/main.cpp
mx2_jpeg_test_LDADD           = libmx.la $(SDL2_LIBS) $(SDL2_TTF_LIBS)

## ── OpenGL examples ────────────────────────────────────────────────────────

mx2_gl_window_SOURCES             = gl_window/window.cpp
mx2_gl_window_LDADD               = libmxgl.la libmx.la

mx2_gl_skeleton_SOURCES           = gl_skeleton/skeleton.cpp
mx2_gl_skeleton_LDADD             = libmxgl.la libmx.la

mx2_gl_example_SOURCES            = gl_example/example.cpp
mx2_gl_example_LDADD              = libmxgl.la libmx.la

mx2_gl_cube_SOURCES               = gl_cube/example.cpp
mx2_gl_cube_LDADD                 = libmxgl.la libmx.la

mx2_gl_pong_SOURCES               = gl_pong/pong.cpp
mx2_gl_pong_LDADD                 = libmxgl.la libmx.la

mx2_gl_animated_cube_SOURCES      = gl_animated_cube/example.cpp
mx2_gl_animated_cube_LDADD        = libmxgl.la libmx.la

mx2_gl_breakout_SOURCES           = gl_breakout/breakout.cpp gl_breakout/game.cpp gl_breakout/intro.cpp
mx2_gl_breakout_LDADD             = libmxgl.la libmx.la

mx2_gl_pyramid_SOURCES            = gl_pyramid/example.cpp
mx2_gl_pyramid_LDADD              = libmxgl.la libmx.la

mx2_gl_mxmod_SOURCES              = gl_mxmod/viewer.cpp
mx2_gl_mxmod_LDADD                = libmxgl.la libmx.la

mx2_space3d_SOURCES               = space3d/explode.cpp space3d/intro.cpp space3d/space.cpp
mx2_space3d_LDADD                 = libmxgl.la libmx.la

mx2_gl_surface_test_SOURCES       = gl_surface_test/window.cpp
mx2_gl_surface_test_LDADD         = libmxgl.la libmx.la

mx2_gl_ps_SOURCES                 = gl_ps/ps.cpp
mx2_gl_ps_LDADD                   = libmxgl.la libmx.la

mx2_gl_ps2_SOURCES                = gl_ps2/ps.cpp
mx2_gl_ps2_LDADD                  = libmxgl.la libmx.la

mx2_gl_ps3_SOURCES                = gl_ps3/ps.cpp
mx2_gl_ps3_LDADD                  = libmxgl.la libmx.la

mx2_gl_ps4_SOURCES                = gl_ps4/ps.cpp
mx2_gl_ps4_LDADD                  = libmxgl.la libmx.la

mx2_gl_ps3d_SOURCES               = gl_ps3d/ps.cpp
mx2_gl_ps3d_LDADD                 = libmxgl.la libmx.la

mx2_gl_stars3d_SOURCES            = gl_stars3d/stars.cpp
mx2_gl_stars3d_LDADD              = libmxgl.la libmx.la

mx2_gl_work_SOURCES               = gl_work/stars.cpp
mx2_gl_work_LDADD                 = libmxgl.la libmx.la

mx2_gl_explode_SOURCES            = gl_explode/explode.cpp gl_explode/test.cpp
mx2_gl_explode_LDADD              = libmxgl.la libmx.la

mx2_gl_walk_SOURCES               = gl_walk/walk.cpp
mx2_gl_walk_LDADD                 = libmxgl.la libmx.la

mx2_gl_walktest_SOURCES           = gl_walktest/room.cpp
mx2_gl_walktest_LDADD             = libmxgl.la libmx.la

mx2_gl_cubemap_SOURCES            = gl_cubemap/example.cpp
mx2_gl_cubemap_LDADD              = libmxgl.la libmx.la

mx2_gl_stencil_test_SOURCES       = gl_stencil_test/skeleton.cpp
mx2_gl_stencil_test_LDADD         = libmxgl.la libmx.la

mx2_gl_blend_shader_SOURCES       = gl_blend_shader/blend.cpp
mx2_gl_blend_shader_LDADD         = libmxgl.la libmx.la

mx2_gl_framebuffer_SOURCES        = gl_framebuffer/skeleton.cpp
mx2_gl_framebuffer_LDADD          = libmxgl.la libmx.la

mx2_gl_shade1_SOURCES             = gl_shade1/shade.cpp
mx2_gl_shade1_LDADD               = libmxgl.la libmx.la

mx2_gl_shade2_SOURCES             = gl_shade2/shade.cpp
mx2_gl_shade2_LDADD               = libmxgl.la libmx.la

mx2_gl_shadow_SOURCES             = gl_shadow/shadow.cpp
mx2_gl_shadow_LDADD               = libmxgl.la libmx.la

mx2_gl_skybox_SOURCES             = gl_skybox/skybox.cpp
mx2_gl_skybox_LDADD               = libmxgl.la libmx.la

mx2_gl_skybox2_SOURCES            = gl_skybox2/skybox.cpp
mx2_gl_skybox2_LDADD              = libmxgl.la libmx.la

mx2_gl_skybox3_SOURCES            = gl_skybox3/skybox.cpp
mx2_gl_skybox3_LDADD              = libmxgl.la libmx.la

mx2_gl_curve_SOURCES              = gl_curve/curve.cpp
mx2_gl_curve_LDADD                = libmxgl.la libmx.la

mx2_gl_snow_SOURCES               = gl_snow/snow.cpp
mx2_gl_snow_LDADD                 = libmxgl.la libmx.la

mx2_gl_fire_SOURCES               = gl_fire/fire.cpp
mx2_gl_fire_LDADD                 = libmxgl.la libmx.la

mx2_gl_water_SOURCES              = gl_water/water.cpp
mx2_gl_water_LDADD                = libmxgl.la libmx.la

mx2_gl_rain_SOURCES               = gl_rain/rain.cpp
mx2_gl_rain_LDADD                 = libmxgl.la libmx.la

mx2_gl_room_SOURCES               = gl_room/room.cpp
mx2_gl_room_LDADD                 = libmxgl.la libmx.la

mx2_gl_matrix_SOURCES             = gl_matrix/matrix.cpp
mx2_gl_matrix_LDADD               = libmxgl.la libmx.la

mx2_gl_obj_explode_SOURCES        = gl_obj_explode/obj_explode.cpp
mx2_gl_obj_explode_LDADD          = libmxgl.la libmx.la

mx2_gl_fog_SOURCES                = gl_fog/room.cpp
mx2_gl_fog_LDADD                  = libmxgl.la libmx.la

mx2_gl_ship_SOURCES               = gl_ship/ship.cpp
mx2_gl_ship_LDADD                 = libmxgl.la libmx.la

mx2_gl_matrix_room_SOURCES        = gl_matrix_room/matrix_room.cpp
mx2_gl_matrix_room_LDADD          = libmxgl.la libmx.la

mx2_gl_matrix_glitch_SOURCES      = gl_matrix_glitch/matrix_glitch.cpp
mx2_gl_matrix_glitch_LDADD        = libmxgl.la libmx.la

mx2_gl_matrix_psyche_SOURCES      = gl_matrix_psyche/matrix_psyche.cpp
mx2_gl_matrix_psyche_LDADD        = libmxgl.la libmx.la

mx2_gl_blank_model_SOURCES        = gl_blank_model/blank_model.cpp
mx2_gl_blank_model_LDADD          = libmxgl.la libmx.la

mx2_gl_intro_screen_SOURCES       = gl_intro_screen/intro.cpp
mx2_gl_intro_screen_LDADD         = libmxgl.la libmx.la

mx2_gl_model_explode_SOURCES      = gl_model_explode/model_explode.cpp
mx2_gl_model_explode_LDADD        = libmxgl.la libmx.la

mx2_gl_model_glitch_SOURCES       = gl_model_glitch/model_glitch.cpp
mx2_gl_model_glitch_LDADD         = libmxgl.la libmx.la

mx2_gl_asteroids_SOURCES          = gl_asteroids/asteroids.cpp
mx2_gl_asteroids_LDADD            = libmxgl.la libmx.la

mx2_gl_console_SOURCES            = gl_console/console_main.cpp
mx2_gl_console_LDADD              = libmxgl.la libmx.la

mx2_gl_console_skeleton_SOURCES   = gl_console_skeleton/console_skeleton.cpp
mx2_gl_console_skeleton_LDADD     = libmxgl.la libmx.la

mx2_gl_console_full_SOURCES       = gl_console_full/console_full.cpp
mx2_gl_console_full_LDADD         = libmxgl.la libmx.la

mx2_gl_object_SOURCES             = gl_object/skeleton.cpp
mx2_gl_object_LDADD               = libmxgl.la libmx.la


mx2_gl_glitch_SOURCES             = gl_glitch/glitch.cpp
mx2_gl_glitch_LDADD               = libmxgl.la libmx.la

mx2_gl_glitch_cube_SOURCES        = gl_glitch_cube/glitch.cpp
mx2_gl_glitch_cube_LDADD          = libmxgl.la libmx.la

mx2_gl_glitch_cube2_SOURCES       = gl_glitch_cube2/glitch.cpp
mx2_gl_glitch_cube2_LDADD         = libmxgl.la libmx.la

mx2_gl_glitch_cube3_SOURCES       = gl_glitch_cube3/glitch.cpp
mx2_gl_glitch_cube3_LDADD         = libmxgl.la libmx.la

mx2_gl_texture_map_SOURCES        = gl_texture_map/texture.cpp
mx2_gl_texture_map_LDADD          = libmxgl.la libmx.la

mx2_gl_glitch_ex_SOURCES          = gl_glitch_ex/model_glitch.cpp
mx2_gl_glitch_ex_LDADD            = libmxgl.la libmx.la

#mx2_gl_about_SOURCES              = gl_about/about.cpp
#mx2_gl_about_LDADD                = libmxgl.la libmx.la

mx2_gl_animation_SOURCES          = gl_animation/animation.cpp gl_animation/binding.cpp
mx2_gl_animation_LDADD            = libmxgl.la libmx.la

mx2_gl_tux_SOURCES                = gl_tux/tux.cpp
mx2_gl_tux_LDADD                  = libmxgl.la libmx.la

## ── OpenGL ES3 examples ────────────────────────────────────────────────────

mx2_gl_es3_SOURCES                = gl_es3/skeleton.cpp
mx2_gl_es3_LDADD                  = libmxgl.la libmx.la

mx2_gl_warp_model_SOURCES         = gl_warp_model/skeleton.cpp
mx2_gl_warp_model_LDADD           = libmxgl.la libmx.la

## ── OpenGL + OpenCV examples ───────────────────────────────────────────────

mx2_gl_cv_skeleton_SOURCES        = gl_cv_skeleton/skeleton.cpp
mx2_gl_cv_skeleton_LDADD          = libmxgl_cv.la libmxgl.la libmx.la $(OPENCV_LIBS)

mx2_gl_walktest_cv_SOURCES        = gl_walktest_cv/room.cpp
mx2_gl_walktest_cv_LDADD          = libmxgl_cv.la libmxgl.la libmx.la $(OPENCV_LIBS)

## ── Vulkan examples ────────────────────────────────────────────────────────

mx2_vk_skeleton_SOURCES           = vk_skeleton/skeleton.cpp
mx2_vk_skeleton_LDADD             = libmxvk.la libvolk.la

mx2_vk_example_SOURCES            = vk_example/main.cpp
mx2_vk_example_LDADD              = libmxvk.la libvolk.la

mx2_vk_tex_cube_SOURCES           = vk_tex_cube/main.cpp vk_tex_cube/vk.cpp
mx2_vk_tex_cube_LDADD             = libmxvk.la libvolk.la

mx2_vk_tex_pyramid_SOURCES        = vk_tex_pyramid/main.cpp vk_tex_pyramid/vk.cpp
mx2_vk_tex_pyramid_LDADD          = libmxvk.la libvolk.la

mx2_vk_data_SOURCES               = vk_data/main.cpp vk_data/vk.cpp
mx2_vk_data_LDADD                 = libmxvk.la libvolk.la

mx2_vk_image_SOURCES              = vk_image/main.cpp vk_image/vk.cpp
mx2_vk_image_LDADD                = libmxvk.la libvolk.la

mx2_vk_frag_SOURCES               = vk_frag/main.cpp vk_frag/vk.cpp
mx2_vk_frag_LDADD                 = libmxvk.la libvolk.la

mx2_vk_borg_SOURCES               = vk_borg/main.cpp vk_borg/vk.cpp
mx2_vk_borg_LDADD                 = libmxvk.la libvolk.la

mx2_vk_wire_SOURCES               = vk_wire/main.cpp vk_wire/vk.cpp
mx2_vk_wire_LDADD                 = libmxvk.la libvolk.la

mx2_vk_text_SOURCES               = vk_text/main.cpp vk_text/vk.cpp
mx2_vk_text_LDADD                 = libmxvk.la libvolk.la

mx2_vk_stars_SOURCES              = vk_stars/main.cpp vk_stars/vk.cpp
mx2_vk_stars_LDADD                = libmxvk.la libvolk.la

mx2_vk_pong_SOURCES               = vk_pong/pong.cpp vk_pong/vk.cpp
mx2_vk_pong_LDADD                 = libmxvk.la libvolk.la

mx2_vk_raycast_SOURCES            = vk_raycast/skeleton.cpp vk_raycast/vk.cpp
mx2_vk_raycast_LDADD              = libmxvk.la libvolk.la

mx2_vk_matrix_SOURCES             = vk_matrix/matrix.cpp vk_matrix/vk.cpp
mx2_vk_matrix_LDADD               = libmxvk.la libvolk.la

mx2_vk_matrix_ray_SOURCES         = vk_matrix_ray/skeleton.cpp vk_matrix_ray/vk.cpp
mx2_vk_matrix_ray_LDADD           = libmxvk.la libvolk.la

mx2_vk_sprite_SOURCES             = vk_sprite/skeleton.cpp
mx2_vk_sprite_LDADD               = libmxvk.la libvolk.la

mx2_vk_point_sprite_SOURCES       = vk_point_sprite/skeleton.cpp
mx2_vk_point_sprite_LDADD         = libmxvk.la libvolk.la

mx2_vk_point_sprite3D_SOURCES     = vk_point_sprite3D/skeleton.cpp
mx2_vk_point_sprite3D_LDADD       = libmxvk.la libvolk.la

mx2_vk_2D_MasterPiece_SOURCES     = vk_2D_MasterPiece/skeleton.cpp
mx2_vk_2D_MasterPiece_LDADD       = libmxvk.la libvolk.la

mx2_vk_spacerox_SOURCES           = vk_spacerox/space.cpp
mx2_vk_spacerox_LDADD             = libmxvk.la libvolk.la

mx2_vk_mxmodel_SOURCES            = vk_mxmodel/main.cpp vk_mxmodel/vk.cpp
mx2_vk_mxmodel_LDADD              = libmxvk.la libvolk.la

mx2_vk_stickball_SOURCES          = vk_stickball/main.cpp
mx2_vk_stickball_LDADD            = libmxvk.la libvolk.la

mx2_vk_abstract_model_SOURCES     = vk_abstract_model/abstract.cpp
mx2_vk_abstract_model_LDADD       = libmxvk.la libvolk.la

mx2_vk_cube_SOURCES               = vk_cube/main.cpp
mx2_vk_cube_LDADD                 = libmxvk.la libvolk.la

#mx2_vk_star3D_SOURCES             = vk_star3D/star.cpp vk_star3D/vk.cpp
#mx2_vk_star3D_LDADD               = libmxvk.la libvolk.la

mx2_vk_tux_SOURCES                = vk_tux/tux.cpp
mx2_vk_tux_LDADD                  = libmxvk.la libvolk.la

## ── Vulkan + OpenCV examples ───────────────────────────────────────────────

mx2_vk_matrix_cv_SOURCES          = vk_matrix_cv/skeleton.cpp vk_matrix_cv/vk.cpp
mx2_vk_matrix_cv_LDADD            = libmxvk.la libvolk.la $(OPENCV_LIBS)

mx2_vk_opencv_SOURCES             = vk_opencv/main.cpp
mx2_vk_opencv_LDADD               = libmxvk_cv.la libmxvk.la libvolk.la $(OPENCV_LIBS)

mx2_vk_opencv_moon_SOURCES        = vk_opencv_moon/main.cpp
mx2_vk_opencv_moon_LDADD          = libmxvk_cv.la libmxvk.la libvolk.la $(OPENCV_LIBS)

mx2_vk_opencv_shader_SOURCES      = vk_opencv_shader/main.cpp
mx2_vk_opencv_shader_LDADD        = libmxvk_cv.la libmxvk.la libvolk.la $(OPENCV_LIBS)

mx2_vk_skybox_SOURCES             = vk_skybox/main.cpp vk_skybox/vk.cpp
mx2_vk_skybox_LDADD               = libmxvk.la libvolk.la $(OPENCV_LIBS)

## ── Cross GL/VK examples ───────────────────────────────────────────────────

EXTRA_mx2_cross_SOURCES               = cross/vk.cpp
EXTRA_mx2_moon_SOURCES                = moon/vk.cpp
EXTRA_mx2_vk_cross_stars_SOURCES      = vk_cross_stars/vk.cpp
EXTRA_mx2_vk_cross_stars_frag_SOURCES = vk_cross_stars_frag/vk.cpp

mx2_cross_SOURCES               = cross/mxcross.cpp cross/skeleton.cpp
mx2_cross_LDADD                 =

mx2_moon_SOURCES                = moon/mxcross.cpp moon/skeleton.cpp
mx2_moon_LDADD                  =

mx2_vk_cross_stars_SOURCES      = vk_cross_stars/mxcross.cpp vk_cross_stars/skeleton.cpp
mx2_vk_cross_stars_LDADD        =

mx2_vk_cross_stars_frag_SOURCES = vk_cross_stars_frag/mxcross.cpp vk_cross_stars_frag/skeleton.cpp
mx2_vk_cross_stars_frag_LDADD   =

if CROSS_USE_VK
mx2_cross_SOURCES               += cross/vk.cpp
mx2_cross_LDADD                 += libmxvk.la libvolk.la
mx2_moon_SOURCES                += moon/vk.cpp
mx2_moon_LDADD                  += libmxvk.la libvolk.la
mx2_vk_cross_stars_SOURCES      += vk_cross_stars/vk.cpp
mx2_vk_cross_stars_LDADD        += libmxvk.la libvolk.la
mx2_vk_cross_stars_frag_SOURCES += vk_cross_stars_frag/vk.cpp
mx2_vk_cross_stars_frag_LDADD   += libmxvk.la libvolk.la
endif

if CROSS_USE_GL
mx2_cross_LDADD                 += libmxgl.la
mx2_moon_LDADD                  += libmxgl.la
mx2_vk_cross_stars_LDADD        += libmxgl.la
mx2_vk_cross_stars_frag_LDADD   += libmxgl.la
endif

## ============================================================================
##  Conditional noinst_PROGRAMS wiring
## ============================================================================

if ENABLE_EXAMPLES

## Base examples (always built with examples)
noinst_PROGRAMS += \
	mx2-test mx2-controller-test mx2-input-test mx2-image-test \
	mx2-knights-tour mx2-pong mx2-asteroids mx2-blocks mx2-piece mx2-matrix \
	mx2-breakout mx2-frogger mx2-mad mx2-space mx2-example

if ENABLE_MIXER
noinst_PROGRAMS += mx2-sound-test
endif

if ENABLE_JPEG
noinst_PROGRAMS += mx2-jpeg-test
endif

if ENABLE_OPENGL
noinst_PROGRAMS += \
	mx2-gl-window mx2-gl-skeleton mx2-gl-example mx2-gl-cube mx2-gl-pong \
	mx2-gl-animated-cube mx2-gl-breakout mx2-gl-pyramid mx2-gl-mxmod \
	mx2-space3d mx2-gl-surface-test \
	mx2-gl-ps mx2-gl-ps2 mx2-gl-ps3 mx2-gl-ps4 mx2-gl-ps3d \
	mx2-gl-stars3d mx2-gl-work mx2-gl-explode mx2-gl-walk mx2-gl-walktest \
	mx2-gl-cubemap mx2-gl-stencil-test mx2-gl-blend-shader mx2-gl-framebuffer \
	mx2-gl-shade1 mx2-gl-shade2 mx2-gl-shadow \
	mx2-gl-skybox mx2-gl-skybox2 mx2-gl-skybox3 \
	mx2-gl-curve mx2-gl-snow mx2-gl-fire mx2-gl-water mx2-gl-rain mx2-gl-room \
	mx2-gl-matrix mx2-gl-obj-explode mx2-gl-fog mx2-gl-ship \
	mx2-gl-matrix-room mx2-gl-matrix-glitch mx2-gl-matrix-psyche \
	mx2-gl-blank-model mx2-gl-intro-screen mx2-gl-model-explode mx2-gl-model-glitch \
	mx2-gl-asteroids mx2-gl-console mx2-gl-console-skeleton mx2-gl-console-full \
	mx2-gl-object \
	mx2-gl-glitch mx2-gl-glitch-cube mx2-gl-glitch-cube2 mx2-gl-glitch-cube3 \
	mx2-gl-texture-map mx2-gl-glitch-ex \
	mx2-gl-about mx2-gl-animation mx2-gl-tux

if ENABLE_ES3
noinst_PROGRAMS += mx2-gl-es3 mx2-gl-warp-model
endif

if ENABLE_OPENCV
noinst_PROGRAMS += mx2-gl-cv-skeleton mx2-gl-walktest-cv
endif

endif ## ENABLE_OPENGL

if ENABLE_VULKAN
noinst_PROGRAMS += \
	mx2-vk-skeleton mx2-vk-example mx2-vk-tex-cube mx2-vk-tex-pyramid \
	mx2-vk-data mx2-vk-image mx2-vk-frag mx2-vk-borg mx2-vk-wire mx2-vk-text \
	mx2-vk-stars mx2-vk-pong mx2-vk-raycast mx2-vk-matrix mx2-vk-matrix-ray \
	mx2-vk-sprite mx2-vk-point-sprite mx2-vk-point-sprite3D \
	mx2-vk-2D-MasterPiece mx2-vk-spacerox mx2-vk-mxmodel mx2-vk-stickball \
	mx2-vk-abstract-model mx2-vk-cube mx2-vk-star3D mx2-vk-tux

if ENABLE_OPENCV
noinst_PROGRAMS += mx2-vk-matrix-cv mx2-vk-opencv mx2-vk-opencv-moon mx2-vk-opencv-shader mx2-vk-skybox
endif

endif ## ENABLE_VULKAN

endif ## ENABLE_EXAMPLES

## Cross examples (independent of ENABLE_EXAMPLES)
if ENABLE_CROSS
if CROSS_USE_VK
noinst_PROGRAMS += mx2-cross mx2-moon mx2-vk-cross-stars mx2-vk-cross-stars-frag
endif
if CROSS_USE_GL
noinst_PROGRAMS += mx2-cross mx2-moon mx2-vk-cross-stars mx2-vk-cross-stars-frag
endif
endif

## ============================================================================
##  Install config.h to the include dir
## ============================================================================
nodist_mx2include_HEADERS = config.h

## ============================================================================
##  Clean helpers
## ============================================================================
CLEANFILES = $(EXTRA_PROGRAMS)

.PHONY: show-config
show-config:
	@echo "libmx2 $(PACKAGE_VERSION) build configuration:"
	@echo "  prefix:   $(prefix)"
	@echo "  OpenGL:   @enable_opengl@"
	@echo "  Vulkan:   @enable_vulkan@"
	@echo "  Mixer:    @enable_mixer@"
	@echo "  JPEG:     @enable_jpeg@"
	@echo "  OpenCV:   @enable_opencv@"
	@echo "  ES3:      @enable_es3@"
	@echo "  Examples: @enable_examples@"
	@echo "  Cross:    @enable_cross@"
