#!/usr/bin/env python3
import argparse
import math

TAU = math.tau

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

def estimate_normal(p, pu, pv):
    n = v_cross(v_sub(pu, p), v_sub(pv, p))
    n = v_norm(n)
    return v_mul(n, -1.0)

def clamp01(x):
    return 0.0 if x < 0.0 else (1.0 if x > 1.0 else x)

def generate_tube_with_caps(length, tube_radius, sphere_radius, seg_len, seg_around, cap_lat):
    verts = []
    uvs = []
    norms = []

    def add_tri(p0, p1, p2, uv0, uv1, uv2, n0, n1, n2):
        verts.extend([p0, p1, p2])
        uvs.extend([uv0, uv1, uv2])
        norms.extend([n0, n1, n2])

    half = length * 0.5

    def tube_pos(t, s):
        z = -half + t * length
        ang = s * TAU
        x = tube_radius * math.cos(ang)
        y = tube_radius * math.sin(ang)
        return (x, y, z)

    def tube_norm(s):
        ang = s * TAU
        return (-math.cos(ang), -math.sin(ang), 0.0)

    for i in range(seg_len):
        t0 = i / seg_len
        t1 = (i + 1) / seg_len
        for j in range(seg_around):
            s0 = j / seg_around
            s1 = (j + 1) / seg_around

            p00 = tube_pos(t0, s0)
            p10 = tube_pos(t1, s0)
            p11 = tube_pos(t1, s1)
            p01 = tube_pos(t0, s1)

            uv00 = (s0, t0)
            uv10 = (s0, t1)
            uv11 = (s1, t1)
            uv01 = (s1, t0)

            n00 = tube_norm(s0)
            n10 = tube_norm(s0)
            n11 = tube_norm(s1)
            n01 = tube_norm(s1)

            add_tri(p00, p10, p11, uv00, uv10, uv11, n00, n10, n11)
            add_tri(p00, p11, p01, uv00, uv11, uv01, n00, n11, n01)

    def sphere_end(center_z, flip):
        for i in range(cap_lat):
            u0 = i / cap_lat
            u1 = (i + 1) / cap_lat
            th0 = u0 * (math.pi * 0.5)
            th1 = u1 * (math.pi * 0.5)

            for j in range(seg_around):
                s0 = j / seg_around
                s1 = (j + 1) / seg_around
                ph0 = s0 * TAU
                ph1 = s1 * TAU

                def sph(th, ph):
                    r = sphere_radius
                    x = r * math.cos(ph) * math.sin(th)
                    y = r * math.sin(ph) * math.sin(th)
                    z = r * math.cos(th)
                    z = -z if flip else z
                    return (x, y, center_z + z)

                p00 = sph(th0, ph0)
                p10 = sph(th1, ph0)
                p11 = sph(th1, ph1)
                p01 = sph(th0, ph1)

                v0 = clamp01((center_z - (-half)) / length)
                v1 = clamp01((center_z - (-half)) / length)

                uv00 = (s0, v0)
                uv10 = (s0, v1)
                uv11 = (s1, v1)
                uv01 = (s1, v0)

                eps_th = (0.5 / max(8, cap_lat)) * (math.pi * 0.5)
                eps_ph = (0.5 / max(8, seg_around)) * TAU

                p00_th = sph(min(math.pi*0.5, th0 + eps_th), ph0)
                p00_ph = sph(th0, ph0 + eps_ph)
                p01_th = sph(min(math.pi*0.5, th0 + eps_th), ph1)
                p01_ph = sph(th0, ph1 + eps_ph)
                p10_th = sph(min(math.pi*0.5, th1 + eps_th), ph0)
                p10_ph = sph(th1, ph0 + eps_ph)
                p11_th = sph(min(math.pi*0.5, th1 + eps_th), ph1)
                p11_ph = sph(th1, ph1 + eps_ph)

                n00 = estimate_normal(p00, p00_th, p00_ph)
                n10 = estimate_normal(p10, p10_th, p10_ph)
                n11 = estimate_normal(p11, p11_th, p11_ph)
                n01 = estimate_normal(p01, p01_th, p01_ph)

                add_tri(p00, p10, p11, uv00, uv10, uv11, n00, n10, n11)
                add_tri(p00, p11, p01, uv00, uv11, uv01, n00, n11, n01)

    sphere_end(-half, flip=True)
    sphere_end(half, flip=False)

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
    ap.add_argument("-o", "--out", default="tube_with_spheres.mxmod")
    ap.add_argument("--length", type=float, default=220.0)
    ap.add_argument("--tube_radius", type=float, default=35.0)
    ap.add_argument("--sphere_radius", type=float, default=55.0)
    ap.add_argument("--seg_len", type=int, default=48)
    ap.add_argument("--seg_around", type=int, default=64)
    ap.add_argument("--cap_lat", type=int, default=16)
    args = ap.parse_args()

    seg_len = max(6, args.seg_len)
    seg_around = max(12, args.seg_around)
    cap_lat = max(6, args.cap_lat)

    verts, uvs, norms = generate_tube_with_caps(
        length=args.length,
        tube_radius=args.tube_radius,
        sphere_radius=args.sphere_radius,
        seg_len=seg_len,
        seg_around=seg_around,
        cap_lat=cap_lat
    )

    write_mxmod(args.out, verts, uvs, norms)
    print(f"Wrote: {args.out}")
    print(f"Triangles: {len(verts)//3}")
    print(f"Vertices: {len(verts)}")

if __name__ == "__main__":
    main()
