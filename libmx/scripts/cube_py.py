#!/usr/bin/env python3
import argparse
import math

def v_sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])

def v_cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def v_len(a):
    return math.sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2])

def v_norm(a):
    l = v_len(a)
    if l <= 1e-12:
        return (0.0, 1.0, 0.0)
    return (a[0]/l, a[1]/l, a[2]/l)

def v_mul(a, s):
    return (a[0]*s, a[1]*s, a[2]*s)

def face_basis(n):
    ax = abs(n[0]); ay = abs(n[1]); az = abs(n[2])
    up = (1.0, 0.0, 0.0) if ax < 0.9 else (0.0, 1.0, 0.0)
    t = v_norm(v_cross(up, n))
    b = v_norm(v_cross(n, t))
    return t, b

def add_tri(verts, uvs, norms, p0, p1, p2, uv0, uv1, uv2):
    n = v_norm(v_cross(v_sub(p1, p0), v_sub(p2, p0)))
    verts.extend([p0, p1, p2])
    uvs.extend([uv0, uv1, uv2])
    norms.extend([n, n, n])

def add_quad(verts, uvs, norms, p00, p10, p11, p01, uv00, uv10, uv11, uv01):
    add_tri(verts, uvs, norms, p00, p10, p11, uv00, uv10, uv11)
    add_tri(verts, uvs, norms, p00, p11, p01, uv00, uv11, uv01)

def add_cube_face(verts, uvs, norms, center, n, half):
    t, b = face_basis(n)
    c = center
    p00 = (c[0] + (-half)*t[0] + (-half)*b[0], c[1] + (-half)*t[1] + (-half)*b[1], c[2] + (-half)*t[2] + (-half)*b[2])
    p10 = (c[0] + ( half)*t[0] + (-half)*b[0], c[1] + ( half)*t[1] + (-half)*b[1], c[2] + ( half)*t[2] + (-half)*b[2])
    p11 = (c[0] + ( half)*t[0] + ( half)*b[0], c[1] + ( half)*t[1] + ( half)*b[1], c[2] + ( half)*t[2] + ( half)*b[2])
    p01 = (c[0] + (-half)*t[0] + ( half)*b[0], c[1] + (-half)*t[1] + ( half)*b[1], c[2] + (-half)*t[2] + ( half)*b[2])

    uv00 = (0.0, 0.0)
    uv10 = (1.0, 0.0)
    uv11 = (1.0, 1.0)
    uv01 = (0.0, 1.0)

    add_quad(verts, uvs, norms, p00, p10, p11, p01, uv00, uv10, uv11, uv01)

def add_pyramid_on_face(verts, uvs, norms, center, n, base_half, height):
    t, b = face_basis(n)
    c = center
    p00 = (c[0] + (-base_half)*t[0] + (-base_half)*b[0], c[1] + (-base_half)*t[1] + (-base_half)*b[1], c[2] + (-base_half)*t[2] + (-base_half)*b[2])
    p10 = (c[0] + ( base_half)*t[0] + (-base_half)*b[0], c[1] + ( base_half)*t[1] + (-base_half)*b[1], c[2] + ( base_half)*t[2] + (-base_half)*b[2])
    p11 = (c[0] + ( base_half)*t[0] + ( base_half)*b[0], c[1] + ( base_half)*t[1] + ( base_half)*b[1], c[2] + ( base_half)*t[2] + ( base_half)*b[2])
    p01 = (c[0] + (-base_half)*t[0] + ( base_half)*b[0], c[1] + (-base_half)*t[1] + ( base_half)*b[1], c[2] + (-base_half)*t[2] + ( base_half)*b[2])

    apex = (c[0] - height*n[0], c[1] - height*n[1], c[2] - height*n[2])

    uv00 = (0.0, 0.0)
    uv10 = (1.0, 0.0)
    uv11 = (1.0, 1.0)
    uv01 = (0.0, 1.0)
    uva = (0.5, 0.5)

    add_tri(verts, uvs, norms, p00, p10, apex, uv00, uv10, uva)
    add_tri(verts, uvs, norms, p10, p11, apex, uv10, uv11, uva)
    add_tri(verts, uvs, norms, p11, p01, apex, uv11, uv01, uva)
    add_tri(verts, uvs, norms, p01, p00, apex, uv01, uv00, uva)

def generate(cube_size, pyramid_height, face_scale):
    verts = []
    uvs = []
    norms = []

    h = cube_size * 0.5
    base_half = h * face_scale

    faces = [
        ((0.0, 0.0,  h), (0.0, 0.0,  1.0)),
        ((0.0, 0.0, -h), (0.0, 0.0, -1.0)),
        (( h, 0.0, 0.0), ( 1.0, 0.0, 0.0)),
        ((-h, 0.0, 0.0), (-1.0, 0.0, 0.0)),
        ((0.0,  h, 0.0), (0.0,  1.0, 0.0)),
        ((0.0, -h, 0.0), (0.0, -1.0, 0.0)),
    ]

    for center, n in faces:
        add_cube_face(verts, uvs, norms, center, n, h)
        add_pyramid_on_face(verts, uvs, norms, center, n, base_half, pyramid_height)

    return verts, uvs, norms

def write_mxmod(path, verts, uvs, norms):
    tri_count = len(verts) // 3
    vert_count = len(verts)
    with open(path, "w", encoding="utf-8") as f:
        f.write(f"tri {tri_count} 0\n")
        f.write(f"vert {vert_count} \n")
        for x, y, z in verts:
            f.write(f"{x:.6f} {y:.6f} {z:.6f}\n")
        f.write(f"\ntex {vert_count}\n")
        for u, v in uvs:
            f.write(f"{u:.6f} {v:.6f}\n")
        f.write(f"\nnorm {vert_count}\n")
        for nx, ny, nz in norms:
            f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default="cube_pyramids_closed.mxmod")
    ap.add_argument("--cube_size", type=float, default=200.0)
    ap.add_argument("--pyramid_height", type=float, default=70.0)
    ap.add_argument("--face_scale", type=float, default=1.0)
    args = ap.parse_args()

    face_scale = args.face_scale
    if face_scale < 0.1:
        face_scale = 0.1
    if face_scale > 1.0:
        face_scale = 1.0

    verts, uvs, norms = generate(args.cube_size, args.pyramid_height, face_scale)
    write_mxmod(args.out, verts, uvs, norms)

    print(f"Wrote: {args.out}")
    print(f"Triangles: {len(verts)//3}")
    print(f"Vertices: {len(verts)}")

if __name__ == "__main__":
    main()
