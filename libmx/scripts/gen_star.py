#!/usr/bin/env python3
import math

def cross_product(a, b):
    """Compute cross product of two 3D vectors a and b."""
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )

def normalize(v):
    """Normalize a 3D vector v."""
    length = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if length < 1e-9:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def generate_star_3d_mxmod_format(
    num_points=5, 
    outer_radius=1.0, 
    inner_radius=0.5, 
    height=1.0
):
    """
    Generate a '3D star' in an mxmod-like format.

    :param num_points:  Number of major points in the star ring (must be >= 2).
    :param outer_radius: Radius of outer 'spikes' in the XY plane.
    :param inner_radius: Radius of the inner vertices in the XY plane.
    :param height:       Half the height of the star (top apex is +height, bottom apex is -height).
    :return: A string containing the model data in mxmod-like format.
    """
    # 1) Generate star ring vertices in the XY plane
    #    We'll alternate outer_radius and inner_radius.
    ring_vertices = []
    angle_step = 2.0 * math.pi / (num_points * 2)  # total 2*num_points segments
    for i in range(num_points * 2):
        # Alternate between outer and inner radii
        r = outer_radius if i % 2 == 0 else inner_radius
        angle = angle_step * i
        x = r * math.cos(angle)
        y = r * math.sin(angle)
        ring_vertices.append((x, y, 0.0))

    # 2) Define top and bottom apex
    top_apex = (0.0, 0.0, +height)
    bottom_apex = (0.0, 0.0, -height)

    # 3) Build the triangles (each triangle is stored as a list of three vertices)
    #    We'll connect each pair of consecutive ring vertices with both apexes.
    triangles = []
    n = len(ring_vertices)
    for i in range(n):
        j = (i + 1) % n  # next index (wrap around)
        v1 = ring_vertices[i]
        v2 = ring_vertices[j]

        # Triangle with the top apex
        triangles.append([v1, v2, top_apex])

        # Triangle with the bottom apex
        triangles.append([v2, v1, bottom_apex])

    # 4) Flatten out all triangles into a single list of vertices
    #    For each triangle, we will assign a normal and texture coords.
    all_verts = []
    all_norms = []
    all_tex = []

    for tri in triangles:
        # tri is [v1, v2, v3]
        v1, v2, v3 = tri

        # Compute face normal
        # We'll do a simplistic face-normal approach: (v2 - v1) x (v3 - v1)
        v12 = (v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2])
        v13 = (v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2])
        face_norm = normalize(cross_product(v12, v13))

        # For each vertex in this triangle, store the same face normal
        for idx, v in enumerate(tri):
            all_verts.append(v)

            # In a simple approach, let's place some basic UV coords:
            # We'll just spread them (0,0), (1,0), (0.5,1.0) for each triangle
            if idx == 0:
                all_tex.append((0.0, 0.0))
            elif idx == 1:
                all_tex.append((1.0, 0.0))
            else:  # idx == 2
                all_tex.append((0.5, 1.0))

            all_norms.append(face_norm)

    # Convert to mxmod-like text
    # tri 0 0 (some type of header line)
    output_lines = []
    output_lines.append("tri 0 0")

    # Vertices
    output_lines.append(f"vert {len(all_verts)}")
    for vx, vy, vz in all_verts:
        output_lines.append(f"{vx:.4f} {vy:.4f} {vz:.4f}")

    # Texture coords
    output_lines.append(f"tex {len(all_tex)}")
    for u, v in all_tex:
        output_lines.append(f"{u:.4f} {v:.4f}")

    # Normals
    output_lines.append(f"norm {len(all_norms)}")
    for nx, ny, nz in all_norms:
        output_lines.append(f"{nx:.4f} {ny:.4f} {nz:.4f}")

    return "\n".join(output_lines)

def main():
    # Generate the mxmod-like data for a 5-point star
    mxmod_data = generate_star_3d_mxmod_format(
        num_points=5,
        outer_radius=1.0,
        inner_radius=0.5,
        height=1.0
    )

    # Print to stdout (you can also write to a file if desired)
    print(mxmod_data)

if __name__ == "__main__":
    main()
