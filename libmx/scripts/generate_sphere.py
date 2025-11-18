import math
import sys

def generate_sphere(radius=0.5, stacks=32, slices=64):
    positions = []
    texcoords = []
    normals = []

    for j in range(stacks + 1):
        v = j / float(stacks)
        theta = v * math.pi
        sin_theta = math.sin(theta)
        cos_theta = math.cos(theta)

        for i in range(slices + 1):
            u = i / float(slices)
            phi = u * 2.0 * math.pi
            sin_phi = math.sin(phi)
            cos_phi = math.cos(phi)

            x = radius * sin_theta * cos_phi
            y = radius * cos_theta
            z = radius * sin_theta * sin_phi

            nx = sin_theta * cos_phi
            ny = cos_theta
            nz = sin_theta * sin_phi

            positions.append((x, y, z))
            normals.append((nx, ny, nz))
            texcoords.append((u, 1.0 - v))

    tri_positions = []
    tri_normals = []
    tri_texcoords = []

    cols = slices + 1

    for j in range(stacks):
        for i in range(slices):
            i0 = j * cols + i
            i1 = j * cols + (i + 1)
            i2 = (j + 1) * cols + (i + 1)
            i3 = (j + 1) * cols + i

            p0 = positions[i0]
            p1 = positions[i1]
            p2 = positions[i2]
            p3 = positions[i3]

            n0 = normals[i0]
            n1 = normals[i1]
            n2 = normals[i2]
            n3 = normals[i3]

            t0 = texcoords[i0]
            t1 = texcoords[i1]
            t2 = texcoords[i2]
            t3 = texcoords[i3]

            tri_positions.extend([p0, p1, p2])
            tri_normals.extend([n0, n1, n2])
            tri_texcoords.extend([t0, t1, t2])

            tri_positions.extend([p0, p2, p3])
            tri_normals.extend([n0, n2, n3])
            tri_texcoords.extend([t0, t2, t3])

    return tri_positions, tri_texcoords, tri_normals

def write_mesh(positions, texcoords, normals, f):
    n = len(positions)
    f.write("tri 0 0\n")
    f.write(f"vert {n}\n")
    for x, y, z in positions:
        f.write(f"{x:.6f} {y:.6f} {z:.6f}\n")
    f.write("\n")
    f.write(f"tex {n}\n")
    for u, v in texcoords:
        f.write(f"{u:.6f} {v:.6f}\n")
    f.write("\n")
    f.write(f"norm {n}\n")
    for nx, ny, nz in normals:
        f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")

if __name__ == "__main__":
    positions, texcoords, normals = generate_sphere(radius=0.5, stacks=32, slices=64)
    write_mesh(positions, texcoords, normals, sys.stdout)
