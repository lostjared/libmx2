#!/usr/bin/env python3
import argparse
import math

TAU = math.tau

def v_sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])

def v_add(a, b):
    return (a[0]+b[0], a[1]+b[1], a[2]+b[2])

def v_mul(a, s):
    return (a[0]*s, a[1]*s, a[2]*s)

def v_cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def v_len(a):
    return math.sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2])

def v_norm(a):
    l = v_len(a)
    if l <= 1e-12:
        return (0.0, 1.0, 0.0)
    return (a[0]/l, a[1]/l, a[2]/l)

def rot_y(x, z, ang):
    c = math.cos(ang)
    s = math.sin(ang)
    return (x*c + z*s, -x*s + z*c)

def rot_x(y, z, ang):
    c = math.cos(ang)
    s = math.sin(ang)
    return (y*c - z*s, y*s + z*c)

def generate_weird_skybox_shell(lat, lon, radius, warp, bands, twist):
    verts = []
    uvs = []
    norms = []

    def surf_pos(t, s):
        theta = t * math.pi
        phi = s * TAU
        st = math.sin(theta)
        ct = math.cos(theta)
        sp = math.sin(phi)
        cp = math.cos(phi)

        r = radius * (1.0 + warp * math.sin(bands * phi) * math.sin((bands + 1) * theta))

        x = r * st * cp
        y = r * ct
        z = r * st * sp

        a = twist * (y / radius)
        x, z = rot_y(x, z, a)

        b = 0.35 * warp * math.sin(phi * (bands + 2))
        y, z = rot_x(y, z, b)

        return (x, y, z)

    def surf_uv(t, s):
        return (s, 1.0 - t)

    def approx_normal(t, s, eps_t, eps_s):
        p = surf_pos(t, s)
        p_t = surf_pos(min(1.0, t + eps_t), s)
        p_s = surf_pos(t, min(1.0, s + eps_s))
        e1 = v_sub(p_t, p)
        e2 = v_sub(p_s, p)
        n = v_norm(v_cross(e1, e2))
        return v_mul(n, -1.0)

    eps_t = 1.0 / max(8, lat) * 0.5
    eps_s = 1.0 / max(8, lon) * 0.5

    for i in range(lat):
        t0 = i / lat
        t1 = (i + 1) / lat
        for j in range(lon):
            s0 = j / lon
            s1 = (j + 1) / lon

            p00 = surf_pos(t0, s0)
            p10 = surf_pos(t1, s0)
            p11 = surf_pos(t1, s1)
            p01 = surf_pos(t0, s1)

            uv00 = surf_uv(t0, s0)
            uv10 = surf_uv(t1, s0)
            uv11 = surf_uv(t1, s1)
            uv01 = surf_uv(t0, s1)

            n00 = approx_normal(t0, s0, eps_t, eps_s)
            n10 = approx_normal(t1, s0, eps_t, eps_s)
            n11 = approx_normal(t1, s1, eps_t, eps_s)
            n01 = approx_normal(t0, s1, eps_t, eps_s)

            verts.extend([p00, p10, p11])
            uvs.extend([uv00, uv10, uv11])
            norms.extend([n00, n10, n11])

            verts.extend([p00, p11, p01])
            uvs.extend([uv00, uv11, uv01])
            norms.extend([n00, n11, n01])

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
    ap.add_argument("-o", "--out", default="weird_skybox.mxmod")
    ap.add_argument("--radius", type=float, default=120.0)
    ap.add_argument("--lat", type=int, default=80)
    ap.add_argument("--lon", type=int, default=160)
    ap.add_argument("--warp", type=float, default=0.28)
    ap.add_argument("--bands", type=int, default=9)
    ap.add_argument("--twist", type=float, default=3.2)
    args = ap.parse_args()

    lat = max(8, args.lat)
    lon = max(8, args.lon)

    verts, uvs, norms = generate_weird_skybox_shell(
        lat=lat,
        lon=lon,
        radius=args.radius,
        warp=args.warp,
        bands=max(1, args.bands),
        twist=args.twist
    )

    write_mxmod(args.out, verts, uvs, norms)
    print(f"Wrote: {args.out}")
    print(f"Triangles: {len(verts)//3}")
    print(f"Vertices: {len(verts)}")

if __name__ == "__main__":
    main()
