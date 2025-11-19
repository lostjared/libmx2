import math

def normalize(v):
    x, y, z = v
    length = math.sqrt(x * x + y * y + z * z)
    if length == 0.0:
        return (0.0, 0.0, 0.0)
    return (x / length, y / length, z / length)

def generate_pyramid(size=1.0, height=1.0):
    half = size / 2.0
    y_base = -height / 2.0
    y_apex = height / 2.0

    v0 = (-half, y_base,  half)  # front-left
    v1 = ( half, y_base,  half)  # front-right
    v2 = ( half, y_base, -half)  # back-right
    v3 = (-half, y_base, -half)  # back-left
    apex = (0.0, y_apex, 0.0)

    tris = [
        # base (two triangles, normal roughly (0,-1,0))
        (v0, v1, v2),
        (v2, v3, v0),
        # sides (front, right, back, left)
        (v0, v1, apex),  # front
        (v1, v2, apex),  # right
        (v2, v3, apex),  # back
        (v3, v0, apex),  # left
    ]

    tex_tris = [
        # base
        ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0)),
        ((1.0, 1.0), (0.0, 1.0), (0.0, 0.0)),
        # sides (triangle mapped to full 0..1 range)
        ((0.0, 0.0), (1.0, 0.0), (0.5, 1.0)),
        ((0.0, 0.0), (1.0, 0.0), (0.5, 1.0)),
        ((0.0, 0.0), (1.0, 0.0), (0.5, 1.0)),
        ((0.0, 0.0), (1.0, 0.0), (0.5, 1.0)),
    ]

    normals = []
    for tri in tris:
        (x0, y0, z0), (x1, y1, z1), (x2, y2, z2) = tri
        ux, uy, uz = (x1 - x0, y1 - y0, z1 - z0)
        vx, vy, vz = (x2 - x0, y2 - y0, z2 - z0)
        nx = uy * vz - uz * vy
        ny = uz * vx - ux * vz
        nz = ux * vy - uy * vx
        n = normalize((nx, ny, nz))
        normals.extend([n, n, n])

    num_verts = len(tris) * 3

    print("tri 0 0")
    print(f"vert {num_verts}")
    for tri in tris:
        for x, y, z in tri:
            print(f"{x:.6f} {y:.6f} {z:.6f}")

    print(f"\ntex {num_verts}")
    for tri_uv in tex_tris:
        for u, v in tri_uv:
            print(f"{u:.6f} {v:.6f}")

    print(f"\nnorm {num_verts}")
    for nx, ny, nz in normals:
        print(f"{nx:.6f} {ny:.6f} {nz:.6f}")

if __name__ == "__main__":
    generate_pyramid()
