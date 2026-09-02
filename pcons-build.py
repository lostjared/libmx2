#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["pcons>=0.24"]
# ///
"""pcons build for libmx2 (https://github.com/lostjared/libmx2).

Mirrors the CMake build rooted at ``libmx/CMakeLists.txt``: three libraries
plus the example programs.

    libmx/libmx/  -> mx      (SDL2 core: window, input, font, image, audio)
                     mxgl    (OpenGL: GLAD, shaders, models, console)  [OPENGL]
                     mx_vk   (Vulkan: pipelines, text, sprites)        [VULKAN]
                     volk    (Vulkan loader)                           [VULKAN]
    libmx/<dir>/  -> one example program per directory

Options (``pcons VAR=value``, or as environment variables). CMake spellings
in parentheses:

    OPENGL=0|1        (-DOPENGL)            OpenGL module          default: on
    VULKAN=0|1        (-DVULKAN)            Vulkan module          default: off
    MOLTEN=0|1        (-DMOLTEN)            Vulkan via MoltenVK    default: off
    MIXER=0|1         (-DMIXER)             SDL2_mixer audio       default: on
    JPEG=0|1          (-DJPEG)              JPEG loading           default: on
    ES3=0|1           (-DES3)               OpenGL ES3 examples    default: off
    COMPUTE=0|1       (-DCOMPUTE)           compute-shader examples default: off
    EXAMPLES=0|1      (-DEXAMPLES)          build the examples     default: on
    STATIC=0|1        (-DBUILD_STATIC_LIB)  static mx/mxgl         default: off
    VARIANT=release   (-DCMAKE_BUILD_TYPE)  release or debug       default: release
    PREFIX=<dir>      (-DCMAKE_PREFIX_PATH) extra dependency search prefix

Install with ``PCONS_INSTALL_PREFIX`` (CMake's ``CMAKE_INSTALL_PREFIX``):

    pcons -B build/pcons PCONS_INSTALL_PREFIX=/usr/local \
        PCONS_FINAL_PREFIX=/usr/local all install

For staged/package installs, ``PCONS_FINAL_PREFIX`` sets the prefix embedded
in pkg-config metadata without changing the staging destination.

Unlike the CMake build, ``install`` also writes pkg-config files
(``libmx.pc``, ``libmxgl.pc``, ``libmxvk.pc``) from the repository's own
``.pc.in`` templates, so downstream projects — acidcam-gpu among them — can
find libmx2 with plain ``pkg-config`` instead of a hand-rolled prefix search.

This script also works as part of a larger build. A project that keeps libmx2
in its tree can pull it in directly, with no install step:

    mx = add_subdirectory("libmx2")
    app.link(mx.mxgl)

Usage:
    pcons -B build/pcons --reconfigure  # configure, generate and build
    pcons -B build/pcons OPENGL=0 MIXER=0  # SDL2 core only
    pcons -B build/pcons EXAMPLES=0        # libraries only
"""

import os
from pathlib import Path

from pcons import (
    ImportedTarget,
    PackageDescription,
    Project,
    Target,
    configure_file,
    find_c_toolchain,
    get_platform,
    get_var,
)

VERSION = "2.2.1"  # project(libmx2 VERSION ...) in libmx/CMakeLists.txt

project_dir = Path(__file__).parent
src = project_dir / "libmx"  # the CMake source directory
platform = get_platform()


def configure_homebrew_paths() -> None:
    """Make direct macOS Pcons invocations see Homebrew's keg-only packages."""
    if not platform.is_macos:
        return
    package_dirs: list[str] = []
    binary_dirs: list[str] = []
    for prefix in (Path("/opt/homebrew"), Path("/usr/local")):
        package_dirs.extend(
            str(path)
            for path in (prefix / "lib" / "pkgconfig", prefix / "share" / "pkgconfig")
            if path.is_dir()
        )
        opt_dir = prefix / "opt"
        if opt_dir.is_dir():
            package_dirs.extend(str(path) for path in opt_dir.glob("*/lib/pkgconfig") if path.is_dir())
            package_dirs.extend(str(path) for path in opt_dir.glob("*/share/pkgconfig") if path.is_dir())
            binary_dirs.extend(str(path) for path in opt_dir.glob("*/bin") if path.is_dir())
    if package_dirs:
        os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(
            package_dirs + [os.environ.get("PKG_CONFIG_PATH", "")]
        )
    if binary_dirs:
        os.environ["PATH"] = os.pathsep.join(binary_dirs + [os.environ.get("PATH", "")])


configure_homebrew_paths()


def option(name: str, default: bool = False) -> bool:
    """Read a boolean build option, mirroring CMake's option()."""
    return get_var(name, "1" if default else "0").lower() in ("1", "on", "true", "yes")


with_opengl = option("OPENGL", default=True)
with_vulkan = option("VULKAN")
with_molten = option("MOLTEN")
with_mixer = option("MIXER", default=True)
with_jpeg = option("JPEG", default=True)
with_es3 = option("ES3")
with_compute = option("COMPUTE")
with_examples = option("EXAMPLES", default=True)
static_libs = option("STATIC")

if with_vulkan and with_molten:
    raise SystemExit(
        "Cannot enable both VULKAN and MOLTEN. Choose one:\n"
        "  VULKAN=1 MOLTEN=0   (loader-based Vulkan)\n"
        "  VULKAN=0 MOLTEN=1   (MoltenVK on macOS)"
    )
any_vulkan = with_vulkan or with_molten

# A prefix given here feeds pkg-config and the header-only searches below.
extra_prefixes = [Path(p) for p in (get_var("PREFIX") or "").split(os.pathsep) if p]
if extra_prefixes:
    os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(
        [str(p / "lib" / "pkgconfig") for p in extra_prefixes]
        + [os.environ.get("PKG_CONFIG_PATH", "")]
    )

search_prefixes = extra_prefixes + [
    Path(p) for p in ("/opt/homebrew", "/usr/local", "/usr") if Path(p).is_dir()
]

# =============================================================================
# Project and toolchain
# =============================================================================

project = Project("libmx2", root_dir=project_dir)

if project.is_top_level:
    env = project.Environment(toolchain=find_c_toolchain())
    env.set_variant(get_var("VARIANT", "release"))
else:
    # Embedded via add_subdirectory(): inherit the parent's compiler and
    # variant, on a copy so the flags below stay local to libmx2.
    env = project.default_environment.clone()

env.cxx.set_standard(20)

# CMake applies these to every target.
env.cc.flags.append("-Wall")
env.cxx.flags.append("-Wall")
if platform.is_linux:
    # CMAKE_POSITION_INDEPENDENT_CODE, needed because mx is shared by default.
    env.cc.flags.append("-fPIC")
    env.cxx.flags.append("-fPIC")

# =============================================================================
# Dependencies
# =============================================================================


def sources_in(directory: Path, *names: str) -> list[Path]:
    """Source paths, relative to *directory*."""
    return [directory / name for name in names]


def find_header_only(name: str, probe: str, hint: str = "") -> ImportedTarget:
    """Locate a header-only dependency that ships no .pc file."""
    for prefix in search_prefixes:
        if (prefix / "include" / probe).exists():
            return ImportedTarget.from_package(
                PackageDescription(
                    name=name,
                    prefix=str(prefix),
                    include_dirs=[str(prefix / "include")],
                )
            )
    raise SystemExit(
        f"{name} not found (looked for include/{probe} under "
        + ", ".join(str(p) for p in search_prefixes)
        + f").\n{hint}Pass its install prefix as PREFIX=<dir>."
    )


sdl2 = project.find_package("sdl2")
sdl2_ttf = project.find_package("SDL2_ttf")
zlib = project.find_package("zlib")
libpng = project.find_package("libpng")

image_libs: list[Target] = [libpng, zlib]
if with_jpeg:
    image_libs.append(project.find_package("libjpeg"))

audio_libs: list[Target] = [project.find_package("SDL2_mixer")] if with_mixer else []

# glm is header-only, and installs CMake config files but no .pc.
glm = (
    [find_header_only("glm", "glm/glm.hpp", hint="Install glm (a math library). ")]
    if with_opengl or any_vulkan
    else []
)

# =============================================================================
# config.h  (configure_file, as in libmx/CMakeLists.txt)
# =============================================================================

config_h = configure_file(
    src / "config.h.in",
    project.build_dir / "config.h",
    {
        "PROJECT_VERSION_MAJOR": VERSION.split(".")[0],
        "PROJECT_VERSION_MINOR": VERSION.split(".")[1],
        "WITH_OPENGL_VAL": "1" if with_opengl else "0",
        "WITH_OPENGL_ES3": "1" if with_es3 else "0",
        "WITH_MIXER_VAL": "1" if with_mixer else "0",
        "WITH_JPEG_VAL": "1" if with_jpeg else "0",
        "WITH_VK_VAL": "1" if with_vulkan else "0",
        "WITH_MOLTEN_VAL": "1" if with_molten else "0",
        "WITH_COMPUTE_VAL": "1" if with_compute else "0",
    },
)

# =============================================================================
# Libraries
# =============================================================================

lib_src = src / "libmx"
Library = project.StaticLibrary if static_libs else project.SharedLibrary


def library_headers(target: Target) -> None:
    """Public include dirs shared by all three modules."""
    # config.h lives in the build directory; the sources include it unqualified.
    target.public.include_dirs.extend([project.build_dir, lib_src, lib_src / "include"])


mx = Library(
    "mx",
    env,
    sources=sources_in(
        lib_src,
        "mx.cpp",
        "util.cpp",
        "loadpng.cpp",
        "font.cpp",
        "cfg.cpp",
        "texture.cpp",
        "exception.cpp",
        "joystick.cpp",
        "jpeg.cpp",
        "input.cpp",
        "sound.cpp",
        "tee_stream.cpp",
    ),
)
library_headers(mx)
mx.link(sdl2, sdl2_ttf, *image_libs, *audio_libs)
if not platform.is_windows:
    mx.public.link_flags.append("-pthread")

libraries: list[Target] = [mx]
modules: dict[str, Target] = {"mx": mx}  # what the examples link against

if with_opengl:
    mxgl = Library(
        "mxgl",
        env,
        sources=sources_in(lib_src, "gl.cpp", "model.cpp", "console.cpp", "src/glad.c"),
    )
    library_headers(mxgl)
    mxgl.public.defines.append("WITH_GL")
    mxgl.link(mx, *glm)
    if platform.is_macos:
        mxgl.public.link_flags.extend(["-framework", "OpenGL"])
    elif platform.is_windows:
        mxgl.public.link_libs.append("opengl32")
    else:
        mxgl.public.link_libs.append("GL")
    libraries.append(mxgl)
    modules["mxgl"] = mxgl

if any_vulkan:
    # Always static, so the volk symbol table is shared with the examples.
    mx_vk = project.StaticLibrary(
        "mx_vk",
        env,
        sources=sources_in(
            lib_src,
            "vk.cpp",
            "vk_model.cpp",
            "vk_abstract_model.cpp",
            "vk_text.cpp",
            "vk_sprite.cpp",
        ),
    )
    library_headers(mx_vk)
    mx_vk.public.include_dirs.append(lib_src / "include" / "volk")
    mx_vk.public.defines.append("WITH_VK")
    mx_vk.link(mx, *glm)

    if with_molten:
        molten = find_header_only(
            "MoltenVK",
            "vulkan/vulkan.h",
            hint="Install MoltenVK (brew install molten-vk). ",
        )
        mx_vk.public.defines.append("WITH_MOLTEN")
        mx_vk.link(molten)
        mx_vk.public.link_libs.append("MoltenVK")
        for framework in (
            "QuartzCore",
            "Metal",
            "IOSurface",
            "CoreGraphics",
            "Foundation",
            "Cocoa",
            "IOKit",
        ):
            mx_vk.public.link_flags.extend(["-framework", framework])
    else:
        volk = project.StaticLibrary(
            "volk", env, sources=sources_in(src / "volk", "volk.cpp")
        )
        volk.public.include_dirs.append(src / "volk")
        volk.link(project.find_package("vulkan"))
        if not platform.is_windows:
            volk.public.link_libs.append("dl")
        mx_vk.link(volk)
        libraries.append(volk)

    libraries.append(mx_vk)
    modules["mx_vk"] = mx_vk

# =============================================================================
# Install
# =============================================================================

# Headers land in <prefix>/include/mx2, matching INSTALL_INCLUDE_DIR.
public_headers = [
    "mx.hpp", "argz.hpp", "util.hpp", "jpeg.hpp", "loadpng.hpp", "object.hpp",
    "font.hpp", "cfg.hpp", "tee_stream.hpp", "texture.hpp", "exception.hpp",
    "joystick.hpp", "wrapper.hpp", "input.hpp", "sound.hpp",
]  # fmt: skip
if with_opengl:
    public_headers += ["gl.hpp", "model.hpp", "console.hpp"]
if any_vulkan:
    public_headers += [
        "vk.hpp", "vk_model.hpp", "vk_abstract_model.hpp", "vk_text.hpp",
        "vk_sprite.hpp",
    ]  # fmt: skip

installed: list[Target] = [
    project.Install("include/mx2", [lib_src / h for h in public_headers] + [config_h]),
    project.Install("lib", libraries),
]
if with_opengl:
    installed += [
        project.Install("include/mx2/glad", [lib_src / "include/glad/glad.h"]),
        project.Install("include/mx2/KHR", [lib_src / "include/KHR/khrplatform.h"]),
    ]

# pkg-config files. The repository ships libmx*.pc.in templates that the CMake
# build never configures or installs, which is why downstream projects have to
# hunt for libmx2 by prefix instead of just asking pkg-config for it. These
# follow the templates, with one correction: gl.hpp includes <glm/glm.hpp>, so
# glm's include directory has to reach anyone compiling against mxgl or mx_vk.
stage_prefix = get_var("PCONS_INSTALL_PREFIX", str(project.root_dir / "dist"))
install_prefix = get_var("PCONS_FINAL_PREFIX", stage_prefix)
glm_cflags = " ".join(f"-I{d}" for t in glm for d in t.public.include_dirs)

# Shared libraries are built with an @rpath/SONAME install name, so consumers
# need a matching runtime search path — CMake's INSTALL_RPATH, in .pc form.
rpath = "" if static_libs or platform.is_windows else " -Wl,-rpath,${libdir}"


def write_pc(
    name: str, description: str, requires: str, libs: str, cflags: str = ""
) -> Path:
    """Write a pkg-config file for one module into the build directory."""
    path = project.build_dir / f"{name}.pc"
    path.write_text(
        f"prefix={install_prefix}\n"
        "exec_prefix=${prefix}\n"
        "libdir=${prefix}/lib\n"
        "includedir=${prefix}/include\n"
        f"\nName: {name}\n"
        f"Description: {description}\n"
        f"Version: {VERSION}\n"
        f"Requires: {requires}\n"
        f"Libs: -L${{libdir}} {libs}\n"
        f"Cflags: -I${{includedir}}/mx2 {cflags}\n".rstrip()
        + "\n"
    )
    return path


pc_paths = [
    write_pc(
        "libmx",
        "MX2 multimedia base library (SDL2 + SDL2_ttf + libpng + zlib)",
        "sdl2 SDL2_ttf libpng zlib" + (" SDL2_mixer" if with_mixer else ""),
        "-lmx" + ("" if platform.is_windows else " -lpthread") + rpath,
    )
]
if with_opengl:
    gl_libs = "-framework OpenGL" if platform.is_macos else "-lGL"
    pc_paths.append(
        write_pc(
            "libmxgl",
            "MX2 OpenGL rendering library",
            "libmx",
            f"-lmxgl {gl_libs}{rpath}",
            glm_cflags,
        )
    )
if any_vulkan:
    pc_paths.append(
        write_pc(
            "libmxvk",
            "MX2 Vulkan rendering library",
            "libmx",
            "-lmx_vk" + ("" if with_molten else " -lvolk -lvulkan"),
            glm_cflags,
        )
    )
installed.append(project.Install("lib/pkgconfig", pc_paths))

# =============================================================================
# Examples
# =============================================================================

# (directory, program, sources) — one program per directory, as upstream.
CORE_EXAMPLES = [
    ("test", "test", ["main.cpp"]),
    ("controller_test", "controller_test", ["main.cpp"]),
    ("input_test", "input_test", ["main.cpp"]),
    ("image_test", "image_test", ["main.cpp"]),
    ("knights_tour", "knights_tour", ["knight.cpp"]),
    ("pong", "pong", ["pong.cpp"]),
    ("asteroids", "asteroids", ["asteroids.cpp", "game.cpp"]),
    ("blocks", "blocks", ["blocks.cpp"]),
    ("piece", "piece", ["piece.cpp"]),
    ("matrix", "matrix", ["matrix.cpp"]),
    ("breakout", "breakout", ["breakout.cpp"]),
    ("frogger", "frogger", ["frogger.cpp"]),
    ("mad", "mad", ["mad.cpp"]),
    ("space", "space", ["space.cpp"]),
]

GL_EXAMPLES = [
    ("gl_window", "gl_window", ["window.cpp"]),
    ("gl_skeleton", "gl_skeleton", ["skeleton.cpp"]),
    ("gl_example", "gl_example", ["example.cpp"]),
    ("gl_cube", "gl_cube", ["example.cpp"]),
    ("gl_pong", "gl_pong", ["pong.cpp"]),
    ("gl_animated_cube", "gl_animated_cube", ["example.cpp"]),
    ("gl_breakout", "gl_breakout", ["breakout.cpp", "intro.cpp", "game.cpp"]),
    ("gl_pyramid", "gl_pyramid", ["example.cpp"]),
    ("gl_mxmod", "gl_mxmod", ["viewer.cpp"]),
    ("gl_surface_test", "gl_surface_test", ["window.cpp"]),
    ("gl_ps", "gl_ps", ["ps.cpp"]),
    ("gl_ps2", "gl_ps2", ["ps.cpp"]),
    ("gl_ps3", "gl_ps3", ["ps.cpp"]),
    ("gl_ps4", "gl_ps4", ["ps.cpp"]),
    ("gl_ps3d", "gl_ps3d", ["ps.cpp"]),
    ("gl_stars3d", "gl_stars3d", ["stars.cpp"]),
    ("gl_work", "gl_work", ["stars.cpp"]),
    ("gl_explode", "gl_explode", ["test.cpp", "explode.cpp"]),
    ("gl_walk", "gl_walk", ["walk.cpp"]),
    ("gl_walktest", "gl_walktest", ["room.cpp"]),
    ("gl_cubemap", "gl_cubemap", ["example.cpp"]),
    ("gl_stencil_test", "gl_stencil_test", ["skeleton.cpp"]),
    ("gl_blend_shader", "gl_blend_shader", ["blend.cpp"]),
    ("gl_framebuffer", "gl_framebuffer", ["skeleton.cpp"]),
    ("gl_shade1", "gl_shade1", ["shade.cpp"]),
    ("gl_shade2", "gl_shade2", ["shade.cpp"]),
    ("gl_shadow", "gl_shadow", ["shadow.cpp"]),
    ("gl_skybox", "gl_skybox", ["skybox.cpp"]),
    ("gl_skybox2", "gl_skybox2", ["skybox.cpp"]),
    ("gl_skybox3", "gl_skybox3", ["skybox.cpp"]),
    ("gl_curve", "gl_curve", ["curve.cpp"]),
    ("gl_snow", "gl_snow", ["snow.cpp"]),
    ("gl_fire", "gl_fire", ["fire.cpp"]),
    ("gl_water", "gl_water", ["water.cpp"]),
    ("gl_rain", "gl_rain", ["rain.cpp"]),
    ("gl_room", "gl_room", ["room.cpp"]),
    ("gl_matrix", "gl_matrix", ["matrix.cpp"]),
    ("gl_obj_explode", "gl_obj_explode", ["obj_explode.cpp"]),
    ("gl_fog", "gl_fog", ["room.cpp"]),
    ("gl_ship", "gl_ship", ["ship.cpp"]),
    ("gl_matrix_room", "gl_matrix_room", ["matrix_room.cpp"]),
    ("gl_matrix_glitch", "gl_matrix_glitch", ["matrix_glitch.cpp"]),
    ("gl_matrix_psyche", "gl_matrix_psyche", ["matrix_psyche.cpp"]),
    ("gl_blank_model", "gl_blank_model", ["blank_model.cpp"]),
    ("gl_intro_screen", "gl_intro_screen", ["intro.cpp"]),
    ("gl_model_explode", "gl_model_explode", ["model_explode.cpp"]),
    ("gl_model_glitch", "gl_model_glitch", ["model_glitch.cpp"]),
    ("gl_console", "gl_console", ["console_main.cpp"]),
    ("gl_console_skeleton", "gl_console_skeleton", ["console_skeleton.cpp"]),
    ("gl_console_full", "gl_console_full", ["console_full.cpp"]),
    ("gl_object", "gl_object_skeleton", ["skeleton.cpp"]),
    # Upstream skips these under MinGW.
    ("gl_glitch", "gl_glitch", ["glitch.cpp"]),
    ("gl_glitch_cube", "gl_glitch_cube", ["glitch.cpp"]),
    ("gl_glitch_cube2", "gl_glitch_cube2", ["glitch.cpp"]),
    ("gl_glitch_cube3", "gl_glitch_cube3", ["glitch.cpp"]),
    ("gl_glitch_ex", "gl_glitch_ex", ["model_glitch.cpp"]),
    ("gl_texture_map", "gl_texture_map", ["texture.cpp"]),
    ("gl_tux", "gl_tux", ["tux.cpp"]),
]

# These call win->mixer unconditionally, so they only compile with MIXER on.
# The CMake build lists them among the plain OpenGL examples, which is why
# -DMIXER=OFF fails there.
GL_MIXER_EXAMPLES = [
    ("space3d", "space3d", ["space.cpp", "intro.cpp", "explode.cpp"]),
    ("gl_asteroids", "gl_asteroids", ["asteroids.cpp"]),
]

ES3_EXAMPLES = [
    ("gl_es3", "gl_es3", ["skeleton.cpp"]),
    ("gl_warp_model", "gl_warp_model", ["skeleton.cpp"]),
]

COMPUTE_EXAMPLES = [("gl_compute", "gl_compute", ["example.cpp"])]

VK_EXAMPLES = [
    ("vk_skeleton", "vk_skeleton", ["skeleton.cpp"]),
    ("vk_example", "vk_example", ["main.cpp"]),
    ("vk_tex_cube", "vk_tex_cube", ["main.cpp", "vk.cpp"]),
    ("vk_tex_pyramid", "vk_tex_pyramid", ["main.cpp", "vk.cpp"]),
    ("vk_data", "vk_data", ["main.cpp", "vk.cpp"]),
    ("vk_image", "vk_image", ["main.cpp", "vk.cpp"]),
    ("vk_frag", "vk_frag", ["main.cpp", "vk.cpp"]),
    ("vk_borg", "vk_borg", ["main.cpp", "vk.cpp"]),
    ("vk_wire", "vk_wire", ["main.cpp", "vk.cpp"]),
    ("vk_text", "vk_text", ["main.cpp", "vk.cpp"]),
    ("vk_stars", "vk_stars", ["main.cpp", "vk.cpp"]),
    ("vk_pong", "vk_pong_game", ["pong.cpp", "vk.cpp"]),
    ("vk_raycast", "vk_raycast", ["skeleton.cpp", "vk.cpp"]),
    ("vk_matrix", "vk_matrix", ["matrix.cpp", "vk.cpp"]),
    ("vk_matrix_ray", "vk_matrix_ray", ["skeleton.cpp", "vk.cpp"]),
    ("vk_sprite", "vk_sprite", ["skeleton.cpp"]),
    ("vk_point_sprite", "vk_point_sprite", ["skeleton.cpp"]),
    ("vk_2D_MasterPiece", "vk_2D_MasterPiece", ["skeleton.cpp"]),
    ("vk_spacerox", "vk_spacerox", ["space.cpp"]),
    ("vk_mxmodel", "vk_mxmodel", ["main.cpp", "vk.cpp"]),
    ("vk_stickball", "vk_stickball", ["main.cpp"]),
    ("vk_tux", "vk_tux", ["tux.cpp"]),
    ("vk_abstract_model", "vk_abstract_model", ["abstract.cpp"]),
]


def build_examples(programs: list[tuple[str, str, list[str]]], lib: Target) -> None:
    """One program per directory, each linked against a single module."""
    for directory, name, sources in programs:
        program = project.Program(
            name, env, sources=sources_in(src / directory, *sources)
        )
        program.link(lib)


if with_examples:
    core = list(CORE_EXAMPLES)
    if with_mixer:
        core.append(("sound_test", "sound_test", ["main.cpp"]))
    if with_jpeg:
        core.append(("jpeg_test", "jpeg_test", ["main.cpp"]))
    build_examples(core, modules["mx"])

    if with_opengl:
        gl = list(GL_EXAMPLES)
        if with_mixer:
            gl += GL_MIXER_EXAMPLES
        if with_es3:
            gl += ES3_EXAMPLES
        if with_compute:
            gl += COMPUTE_EXAMPLES
        build_examples(gl, modules["mxgl"])

    if any_vulkan:
        build_examples(VK_EXAMPLES, modules["mx_vk"])

# A standalone tool: compresses .mxmod models. It uses libmx's headers but
# links none of it, so it takes the include directory without the library.
mxmod_compress = project.Program(
    "mxmod_compress", env, sources=sources_in(src / "mxmod_compress", "cmp.cpp")
)
mxmod_compress.private.include_dirs.append(lib_src)
mxmod_compress.link(zlib)
installed.append(project.Install("bin", [mxmod_compress]))

# `ninja install` stages everything under PCONS_INSTALL_PREFIX (default: dist/).
project.Alias("install", *installed)
