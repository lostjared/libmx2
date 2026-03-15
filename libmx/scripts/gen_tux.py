#!/usr/bin/env python3
"""Generate an improved Tux the Linux Penguin OBJ model - higher poly, better proportions."""

import math
import os
from PIL import Image, ImageDraw

DATA_DIR = "."


def write_obj(filename, groups):
    """Write OBJ file with multiple named groups."""
    with open(filename, "w") as f:
        f.write("# Tux the Linux Penguin - Improved Model\n")
        f.write("# High-poly with multiple texture groups\n\n")

        v_offset = 0
        vt_offset = 0
        vn_offset = 0

        for name, verts, uvs, norms, faces in groups:
            f.write(f"# {name}\n")
            f.write(f"g {name}\n")
            f.write(f"usemtl {name}\n")

            for v in verts:
                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            for uv in uvs:
                f.write(f"vt {uv[0]:.6f} {uv[1]:.6f}\n")
            for n in norms:
                f.write(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")

            for face in faces:
                parts = []
                for vi, ti, ni in face:
                    parts.append(f"{vi + v_offset}/{ti + vt_offset}/{ni + vn_offset}")
                f.write("f " + " ".join(parts) + "\n")

            v_offset += len(verts)
            vt_offset += len(uvs)
            vn_offset += len(norms)
            f.write("\n")


def make_ellipsoid(cx, cy, cz, rx, ry, rz, stacks, slices):
    """Generate ellipsoid with proper pole fans and UV seam vertex."""
    verts = []
    uvs = []
    normals = []
    faces = []
    cols = slices + 1  # extra column for UV seam

    # Top pole vertex (index 1, 1-based)
    verts.append((cx, cy + ry, cz))
    uvs.append((0.5, 0.0))
    normals.append((0.0, 1.0, 0.0))

    # Middle rings: stacks-1 rings (i=1..stacks-1), cols verts each
    for i in range(1, stacks):
        phi = math.pi * i / stacks
        for j in range(cols):
            theta = 2.0 * math.pi * j / slices

            nx = math.sin(phi) * math.cos(theta)
            ny = math.cos(phi)
            nz = math.sin(phi) * math.sin(theta)

            x = cx + rx * nx
            y = cy + ry * ny
            z = cz + rz * nz

            verts.append((x, y, z))
            uvs.append((j / slices, i / stacks))
            normals.append((nx, ny, nz))

    # Bottom pole vertex
    verts.append((cx, cy - ry, cz))
    uvs.append((0.5, 1.0))
    normals.append((0.0, -1.0, 0.0))

    # Top cap: triangle fan from pole to first ring
    for j in range(slices):
        v0 = 1
        v1 = 2 + j
        v2 = 2 + j + 1
        faces.append([(v0, v0, v0), (v1, v1, v1), (v2, v2, v2)])

    # Middle quads
    for i in range(stacks - 2):
        for j in range(slices):
            a = 2 + i * cols + j
            b = a + 1
            c = 2 + (i + 1) * cols + j
            d = c + 1
            faces.append([(a, a, a), (c, c, c), (b, b, b)])
            faces.append([(b, b, b), (c, c, c), (d, d, d)])

    # Bottom cap: triangle fan from last ring to bottom pole
    bot = len(verts)
    last_ring_start = 2 + (stacks - 2) * cols
    for j in range(slices):
        v1 = last_ring_start + j
        v2 = last_ring_start + j + 1
        faces.append([(v1, v1, v1), (bot, bot, bot), (v2, v2, v2)])

    return verts, uvs, normals, faces


def make_pear_body(cx, cy, cz, rx_top, rx_bot, ry, rz_top, rz_bot, stacks, slices):
    """Generate pear/egg body with proper pole fans and UV seam vertex."""
    verts = []
    uvs = []
    normals = []
    faces = []
    cols = slices + 1

    # Top pole
    verts.append((cx, cy + ry, cz))
    uvs.append((0.5, 0.0))
    normals.append((0.0, 1.0, 0.0))

    # Middle rings
    for i in range(1, stacks):
        phi = math.pi * i / stacks
        t = i / stacks
        blend = t * t * (3 - 2 * t)
        cur_rx = rx_top + (rx_bot - rx_top) * blend
        cur_rz = rz_top + (rz_bot - rz_top) * blend

        for j in range(cols):
            theta = 2.0 * math.pi * j / slices

            sp = math.sin(phi)
            cp = math.cos(phi)
            ct = math.cos(theta)
            st = math.sin(theta)

            x = cx + cur_rx * sp * ct
            y = cy + ry * cp
            z = cz + cur_rz * sp * st

            nx = sp * ct
            ny = cp
            nz = sp * st

            verts.append((x, y, z))
            uvs.append((j / slices, t))
            normals.append((nx, ny, nz))

    # Bottom pole
    verts.append((cx, cy - ry, cz))
    uvs.append((0.5, 1.0))
    normals.append((0.0, -1.0, 0.0))

    # Top cap fan
    for j in range(slices):
        v0 = 1
        v1 = 2 + j
        v2 = 2 + j + 1
        faces.append([(v0, v0, v0), (v1, v1, v1), (v2, v2, v2)])

    # Middle quads
    for i in range(stacks - 2):
        for j in range(slices):
            a = 2 + i * cols + j
            b = a + 1
            c = 2 + (i + 1) * cols + j
            d = c + 1
            faces.append([(a, a, a), (c, c, c), (b, b, b)])
            faces.append([(b, b, b), (c, c, c), (d, d, d)])

    # Bottom cap fan
    bot = len(verts)
    last_ring_start = 2 + (stacks - 2) * cols
    for j in range(slices):
        v1 = last_ring_start + j
        v2 = last_ring_start + j + 1
        faces.append([(v1, v1, v1), (bot, bot, bot), (v2, v2, v2)])

    return verts, uvs, normals, faces


def make_flipper(cx, cy, cz, length, width, thickness, angle_deg,
                  side="left", stacks=24, slices=20):
    """Generate a flipper/wing that connects to the body.
    
    The base is positioned so it overlaps with the body surface,
    tapering smoothly to a rounded tip.
    """
    verts = []
    uvs = []
    normals = []
    faces = []

    angle = math.radians(angle_deg)
    sign = -1.0 if side == "left" else 1.0

    def transform_point(lx, ly, lz):
        rx2 = lx * math.cos(angle) - ly * math.sin(angle)
        ry2 = lx * math.sin(angle) + ly * math.cos(angle)
        return cx + sign * rx2, cy + ry2, cz + lz

    cols = slices + 1

    # Ring vertices along flipper length
    for i in range(stacks + 1):
        t = i / stacks

        # Width tapers from full at base to 30% at tip, with rounded end
        taper = 1.0 - t * 0.7
        if t > 0.8:
            tip_t = (t - 0.8) / 0.2
            taper *= 1.0 - tip_t * tip_t * 0.7
        cur_width = width * taper

        # Thickness tapers gently
        cur_thick = thickness * (1.0 - t * 0.5)
        if t > 0.85:
            tip_t = (t - 0.85) / 0.15
            cur_thick *= 1.0 - tip_t * 0.4

        local_y = -length * t

        for j in range(cols):
            theta = 2.0 * math.pi * j / slices
            lx = cur_width * math.cos(theta)
            lz = cur_thick * math.sin(theta)
            x, y, z = transform_point(lx, local_y, lz)

            fnx = math.cos(theta)
            fnz = math.sin(theta)

            verts.append((x, y, z))
            uvs.append((j / slices, t))
            normals.append((fnx * sign, 0.0, fnz))

    # Tube side faces
    for i in range(stacks):
        for j in range(slices):
            a = 1 + i * cols + j
            b = a + 1
            c = 1 + (i + 1) * cols + j
            d = c + 1
            faces.append([(a, a, a), (c, c, c), (b, b, b)])
            faces.append([(b, b, b), (c, c, c), (d, d, d)])

    # Base dome (hemisphere so it blends into body)
    dome_stacks = 8
    dome_height = width * 0.8  # how far the dome bulges inward toward body
    base_ring_start = 1  # first ring of the tube (t=0)

    # Build dome rings from base ring inward to pole
    dome_ring_offsets = []  # track vertex offsets for each dome ring
    prev_ring_start = base_ring_start  # ring 0 = existing base ring of tube

    for di in range(1, dome_stacks):
        dt = di / dome_stacks
        phi = (math.pi / 2.0) * dt  # 0 at base edge, pi/2 at pole
        ring_scale = math.cos(phi)   # shrinks from 1.0 to 0
        dome_y = dome_height * math.sin(phi)  # how far "up" into body

        ring_start = len(verts) + 1
        dome_ring_offsets.append(ring_start)

        for j in range(cols):
            theta = 2.0 * math.pi * j / slices
            lx = width * ring_scale * math.cos(theta)
            lz = thickness * ring_scale * math.sin(theta)
            x, y, z = transform_point(lx, dome_y, lz)

            # Normal points outward on the dome surface
            dnx = math.cos(theta) * math.cos(phi)
            dny_local = math.sin(phi)
            dnz = math.sin(theta) * math.cos(phi)
            # Transform normal direction through the angle rotation
            rnx = dnx * math.cos(angle) - dny_local * math.sin(angle)
            rny = dnx * math.sin(angle) + dny_local * math.cos(angle)

            verts.append((x, y, z))
            uvs.append((j / slices, 0.0))
            normals.append((rnx * sign, rny, dnz))

    # Dome pole vertex
    dome_pole_idx = len(verts) + 1
    px, py, pz = transform_point(0, dome_height, 0)
    verts.append((px, py, pz))
    uvs.append((0.5, 0.0))
    # Pole normal points straight into body
    pnx = -math.sin(angle)
    pny = math.cos(angle)
    normals.append((pnx * sign, pny, 0.0))

    # Connect base ring (tube ring 0) to first dome ring
    if len(dome_ring_offsets) > 0:
        first_dome = dome_ring_offsets[0]
        for j in range(slices):
            a = base_ring_start + j
            b = a + 1
            c = first_dome + j
            d = c + 1
            # Wind so faces point outward (away from body center)
            faces.append([(a, a, a), (b, b, b), (c, c, c)])
            faces.append([(b, b, b), (d, d, d), (c, c, c)])

    # Connect successive dome rings
    for di in range(len(dome_ring_offsets) - 1):
        r1 = dome_ring_offsets[di]
        r2 = dome_ring_offsets[di + 1]
        for j in range(slices):
            a = r1 + j
            b = a + 1
            c = r2 + j
            d = c + 1
            faces.append([(a, a, a), (b, b, b), (c, c, c)])
            faces.append([(b, b, b), (d, d, d), (c, c, c)])

    # Connect last dome ring to pole
    if len(dome_ring_offsets) > 0:
        last_dome = dome_ring_offsets[-1]
    else:
        last_dome = base_ring_start
    for j in range(slices):
        v1 = last_dome + j
        v2 = last_dome + j + 1
        faces.append([(v1, v1, v1), (v2, v2, v2), (dome_pole_idx, dome_pole_idx, dome_pole_idx)])

    # Tip cap
    tip_center_idx = len(verts) + 1
    tx, ty, tz = transform_point(0, -length, 0)
    verts.append((tx, ty, tz))
    uvs.append((0.5, 1.0))
    normals.append((0.0, -sign * math.sin(angle) - math.cos(angle), 0.0))
    tip_ring_start = 1 + stacks * cols
    for j in range(slices):
        v1 = tip_ring_start + j
        v2 = tip_ring_start + j + 1
        faces.append([(tip_center_idx, tip_center_idx, tip_center_idx), (v1, v1, v1), (v2, v2, v2)])

    return verts, uvs, normals, faces


def make_foot(cx, cy, cz, length, width, height, toe_spread, side="left", stacks=16, slices=20):
    """Generate foot with end caps (no holes)."""
    verts = []
    uvs = []
    normals = []
    faces = []

    sign = -1.0 if side == "left" else 1.0

    def foot_ring(t):
        if t < 0.7:
            wf = 1.0 + toe_spread * (t / 0.7)
        else:
            wf = (1.0 + toe_spread) * (1.0 - (t - 0.7) / 0.3) * 0.7 + 0.3
        return width * wf, height * (1.0 - t * 0.3)

    cols = slices + 1

    # Ring vertices
    for i in range(stacks + 1):
        t = i / stacks
        cur_w, cur_h = foot_ring(t)
        for j in range(cols):
            theta = 2.0 * math.pi * j / slices
            lx = cur_w * math.cos(theta)
            ly = cur_h * math.sin(theta)
            lz = length * t

            x = cx + sign * lx * 0.5 + sign * 0.3 * t
            y = cy + ly
            z = cz + lz

            fnx = math.cos(theta)
            fny = math.sin(theta)

            verts.append((x, y, z))
            uvs.append((j / slices, t))
            normals.append((fnx * sign, fny, 0.0))

    # Tube side faces
    for i in range(stacks):
        for j in range(slices):
            a = 1 + i * cols + j
            b = a + 1
            c = 1 + (i + 1) * cols + j
            d = c + 1
            faces.append([(a, a, a), (c, c, c), (b, b, b)])
            faces.append([(b, b, b), (c, c, c), (d, d, d)])

    # Heel cap (t=0)
    heel_idx = len(verts) + 1
    verts.append((cx, cy, cz))
    uvs.append((0.5, 0.0))
    normals.append((0.0, 0.0, -1.0))
    for j in range(slices):
        v1 = 1 + j
        v2 = 1 + j + 1
        faces.append([(heel_idx, heel_idx, heel_idx), (v2, v2, v2), (v1, v1, v1)])

    # Toe cap (t=1)
    toe_idx = len(verts) + 1
    verts.append((cx + sign * 0.3, cy, cz + length))
    uvs.append((0.5, 1.0))
    normals.append((0.0, 0.0, 1.0))
    toe_ring_start = 1 + stacks * cols
    for j in range(slices):
        v1 = toe_ring_start + j
        v2 = toe_ring_start + j + 1
        faces.append([(toe_idx, toe_idx, toe_idx), (v1, v1, v1), (v2, v2, v2)])

    return verts, uvs, normals, faces


def generate_tux():
    """Generate improved Tux model with better proportions matching reference."""
    groups = []
    stk = 36
    slc = 36

    # === BODY (black) - pear-shaped, chubby ===
    body_v, body_uv, body_n, body_f = make_pear_body(
        0, 2.0, 0,
        rx_top=1.8, rx_bot=2.8,
        ry=3.8,
        rz_top=1.5, rz_bot=2.3,
        stacks=stk, slices=slc
    )
    groups.append(("body", body_v, body_uv, body_n, body_f))

    # === BELLY (white) - large oval on front ===
    belly_v, belly_uv, belly_n, belly_f = make_ellipsoid(
        0, 1.5, 1.4, 1.7, 3.0, 0.8,
        stacks=stk, slices=slc
    )
    groups.append(("belly", belly_v, belly_uv, belly_n, belly_f))

    # === HEAD (black) - large, round ===
    head_v, head_uv, head_n, head_f = make_ellipsoid(
        0, 6.2, 0, 2.2, 2.2, 2.0,
        stacks=stk, slices=slc
    )
    groups.append(("head", head_v, head_uv, head_n, head_f))

    # === FACE (white eye area) ===
    face_v, face_uv, face_n, face_f = make_ellipsoid(
        0, 6.4, 1.0, 1.7, 1.5, 0.7,
        stacks=28, slices=28
    )
    groups.append(("face", face_v, face_uv, face_n, face_f))

    # === LEFT EYE (white sclera) - large, centered ===
    leye_v, leye_uv, leye_n, leye_f = make_ellipsoid(
        -0.55, 6.7, 1.65, 0.50, 0.55, 0.30,
        stacks=24, slices=24
    )
    groups.append(("left_eye", leye_v, leye_uv, leye_n, leye_f))

    # === RIGHT EYE ===
    reye_v, reye_uv, reye_n, reye_f = make_ellipsoid(
        0.55, 6.7, 1.65, 0.50, 0.55, 0.30,
        stacks=24, slices=24
    )
    groups.append(("right_eye", reye_v, reye_uv, reye_n, reye_f))

    # === LEFT PUPIL ===
    lpupil_v, lpupil_uv, lpupil_n, lpupil_f = make_ellipsoid(
        -0.50, 6.75, 1.92, 0.22, 0.25, 0.12,
        stacks=16, slices=16
    )
    groups.append(("left_pupil", lpupil_v, lpupil_uv, lpupil_n, lpupil_f))

    # === RIGHT PUPIL ===
    rpupil_v, rpupil_uv, rpupil_n, rpupil_f = make_ellipsoid(
        0.50, 6.75, 1.92, 0.22, 0.25, 0.12,
        stacks=16, slices=16
    )
    groups.append(("right_pupil", rpupil_v, rpupil_uv, rpupil_n, rpupil_f))

    # === UPPER BEAK (golden yellow) ===
    beak_upper_v, beak_upper_uv, beak_upper_n, beak_upper_f = make_ellipsoid(
        0, 6.05, 2.1, 0.65, 0.25, 0.8,
        stacks=20, slices=20
    )
    groups.append(("beak_upper", beak_upper_v, beak_upper_uv, beak_upper_n, beak_upper_f))

    # === LOWER BEAK ===
    beak_lower_v, beak_lower_uv, beak_lower_n, beak_lower_f = make_ellipsoid(
        0, 5.85, 2.0, 0.55, 0.18, 0.7,
        stacks=20, slices=20
    )
    groups.append(("beak_lower", beak_lower_v, beak_lower_uv, beak_lower_n, beak_lower_f))

    # === LEFT FOOT ===
    lfoot_v, lfoot_uv, lfoot_n, lfoot_f = make_foot(
        -1.2, -1.75, -0.3,
        length=2.5, width=1.4, height=0.3,
        toe_spread=0.8, side="left", stacks=20, slices=24
    )
    groups.append(("left_foot", lfoot_v, lfoot_uv, lfoot_n, lfoot_f))

    # === RIGHT FOOT ===
    rfoot_v, rfoot_uv, rfoot_n, rfoot_f = make_foot(
        1.2, -1.75, -0.3,
        length=2.5, width=1.4, height=0.3,
        toe_spread=0.8, side="right", stacks=20, slices=24
    )
    groups.append(("right_foot", rfoot_v, rfoot_uv, rfoot_n, rfoot_f))

    # === LEFT WING/FLIPPER (base overlaps body for connected look) ===
    lwing_v, lwing_uv, lwing_n, lwing_f = make_flipper(
        -2.0, 3.5, 0,
        length=2.8, width=0.9, thickness=0.35,
        angle_deg=20, side="left", stacks=24, slices=20
    )
    groups.append(("left_wing", lwing_v, lwing_uv, lwing_n, lwing_f))

    # === RIGHT WING/FLIPPER (base overlaps body for connected look) ===
    rwing_v, rwing_uv, rwing_n, rwing_f = make_flipper(
        2.0, 3.5, 0,
        length=2.8, width=0.9, thickness=0.35,
        angle_deg=20, side="right", stacks=24, slices=20
    )
    groups.append(("right_wing", rwing_v, rwing_uv, rwing_n, rwing_f))

    return groups


# ─── Texture generators ───

def make_body_texture(filename, size=512):
    """Black body with subtle glossy sheen."""
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        for x in range(size):
            cx2, cy2 = size / 2, size / 2
            dx = (x - cx2) / cx2
            dy = (y - cy2) / cy2
            dist = math.sqrt(dx * dx + dy * dy)
            highlight = max(0, int(30 * (1.0 - dist)))
            r = min(255, 12 + highlight)
            g = min(255, 12 + highlight)
            b = min(255, 16 + highlight)
            img.putpixel((x, y), (r, g, b, 255))
    img.save(filename)
    print(f"  {filename}")


def make_belly_texture(filename, size=512):
    """White belly with soft subtle shading."""
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        for x in range(size):
            cx2, cy2 = size / 2, size / 2
            dx = (x - cx2) / cx2
            dy = (y - cy2) / cy2
            dist = math.sqrt(dx * dx + dy * dy)
            factor = max(0.92, 1.0 - 0.08 * dist)
            shade = int(248 * factor)
            img.putpixel((x, y), (shade, shade, min(255, shade + 3), 255))
    img.save(filename)
    print(f"  {filename}")


def make_face_texture(filename, size=512):
    """White face patch texture."""
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        factor = 0.94 + 0.06 * (1.0 - y / size)
        shade = int(245 * factor)
        for x in range(size):
            img.putpixel((x, y), (shade, shade, min(255, shade + 3), 255))
    img.save(filename)
    print(f"  {filename}")


def make_eye_texture(filename, size=512):
    """White eye sclera."""
    img = Image.new("RGBA", (size, size), (255, 255, 255, 255))
    cx2, cy2 = size // 2, size // 2
    for y in range(size):
        for x in range(size):
            dx = (x - cx2) / (size / 2)
            dy = (y - cy2) / (size / 2)
            dist = math.sqrt(dx * dx + dy * dy)
            if dist < 1.0:
                shade = int(255 - 20 * dist)
                img.putpixel((x, y), (shade, shade, shade, 255))
            else:
                img.putpixel((x, y), (240, 240, 240, 255))
    img.save(filename)
    print(f"  {filename}")


def make_pupil_texture(filename, size=256):
    """Black pupil with white highlight reflection."""
    img = Image.new("RGBA", (size, size), (5, 5, 8, 255))
    cx2, cy2 = size // 2, size // 2
    for y in range(size):
        for x in range(size):
            dx = (x - cx2) / (size / 2)
            dy = (y - cy2) / (size / 2)
            dist = math.sqrt(dx * dx + dy * dy)
            base = int(8 + 12 * dist)
            img.putpixel((x, y), (min(base, 30), min(base, 30), min(base + 5, 35), 255))

    # White highlight
    hl_r = size // 6
    hx, hy = cx2 - size // 5, cy2 - size // 5
    for y2 in range(max(0, hy - hl_r), min(size, hy + hl_r)):
        for x2 in range(max(0, hx - hl_r), min(size, hx + hl_r)):
            dx = (x2 - hx) / hl_r
            dy = (y2 - hy) / hl_r
            dist = dx * dx + dy * dy
            if dist < 1.0:
                alpha = int(220 * (1.0 - dist))
                img.putpixel((x2, y2), (255, 255, 255, max(alpha, 50)))
    img.save(filename)
    print(f"  {filename}")


def make_beak_texture(filename, size=512):
    """Golden yellow beak texture."""
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        for x in range(size):
            cx2, cy2 = size / 2, size / 2
            dx = (x - cx2) / cx2
            dy = (y - cy2) / cy2
            dist = math.sqrt(dx * dx + dy * dy)
            factor = max(0.65, 1.0 - 0.35 * dist)
            r = min(255, int(255 * factor))
            g = min(255, int(200 * factor))
            b = min(255, int(30 * factor))
            img.putpixel((x, y), (r, g, b, 255))
    img.save(filename)
    print(f"  {filename}")


def make_foot_texture(filename, size=512):
    """Yellow/orange foot texture."""
    img = Image.new("RGBA", (size, size))
    draw = ImageDraw.Draw(img)
    for y in range(size):
        factor = 0.85 + 0.15 * (1.0 - y / size)
        for x in range(size):
            r = min(255, int(255 * factor))
            g = min(255, int(190 * factor))
            b = min(255, int(20 * factor + 10))
            img.putpixel((x, y), (r, g, b, 255))
    for i in range(4):
        x_start = size // 5 + i * size // 5
        draw.line([(x_start, size // 5), (size // 2, size * 4 // 5)],
                  fill=(220, 150, 10, 255), width=4)
    img.save(filename)
    print(f"  {filename}")


def make_wing_texture(filename, size=512):
    """Very dark wing texture."""
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        for x in range(size):
            cx2, cy2 = size / 2, size / 2
            dx = (x - cx2) / cx2
            dy = (y - cy2) / cy2
            dist = math.sqrt(dx * dx + dy * dy)
            highlight = max(0, int(18 * (1.0 - dist)))
            r = min(255, 8 + highlight)
            g = min(255, 8 + highlight)
            b = min(255, 14 + highlight)
            img.putpixel((x, y), (r, g, b, 255))
    img.save(filename)
    print(f"  {filename}")


def main():
    tex_dir = os.path.join(DATA_DIR, "tex")
    os.makedirs(tex_dir, exist_ok=True)

    print("Generating improved Tux model (high poly)...")
    groups = generate_tux()

    obj_path = os.path.join(DATA_DIR, "tux.obj")
    write_obj(obj_path, groups)

    total_verts = sum(len(g[1]) for g in groups)
    total_faces = sum(len(g[4]) for g in groups)
    print(f"Created: {obj_path}")
    print(f"  {len(groups)} mesh groups, {total_verts} vertices, {total_faces} triangles")

    print("\nGenerating textures...")
    texture_map = [
        ("tex/tux_body.png",       make_body_texture,  512),
        ("tex/tux_belly.png",      make_belly_texture, 512),
        ("tex/tux_head.png",       make_body_texture,  512),
        ("tex/tux_face.png",       make_face_texture,  512),
        ("tex/tux_eye_l.png",      make_eye_texture,   512),
        ("tex/tux_eye_r.png",      make_eye_texture,   512),
        ("tex/tux_pupil_l.png",    make_pupil_texture, 256),
        ("tex/tux_pupil_r.png",    make_pupil_texture, 256),
        ("tex/tux_beak_upper.png", make_beak_texture,  512),
        ("tex/tux_beak_lower.png", make_beak_texture,  512),
        ("tex/tux_foot_l.png",     make_foot_texture,  512),
        ("tex/tux_foot_r.png",     make_foot_texture,  512),
        ("tex/tux_wing_l.png",     make_wing_texture,  512),
        ("tex/tux_wing_r.png",     make_wing_texture,  512),
    ]

    texture_filenames = []
    for tex_name, gen_func, sz in texture_map:
        full_path = os.path.join(DATA_DIR, tex_name)
        gen_func(full_path, sz)
        texture_filenames.append(tex_name)

    tex_path = os.path.join(DATA_DIR, "tux.tex")
    with open(tex_path, "w") as f:
        for t in texture_filenames:
            f.write(t + "\n")

    print(f"\nCreated: {tex_path} ({len(texture_filenames)} textures)")
    print("\nDone! Use: tux.obj with tux.tex")


if __name__ == "__main__":
    main()
