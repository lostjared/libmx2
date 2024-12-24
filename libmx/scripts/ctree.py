#!/usr/bin/env python3

import sys
import math

def generate_christmas_tree_simplified(
    trunk_radius=0.1,
    trunk_height=0.3,
    tree_base_radius=0.5,
    tree_height=1.0,
    slices=4
):
    """
    Generate a simple 3D model of a Christmas tree (trunk + conical top)
    but store faces as index triplets (v1, v2, v3).

    Returns:
        vertices: list of (x, y, z)
        faces: list of (i1, i2, i3) where each iN is an integer index (1-based)
               referring to a vertex in 'vertices'.
    """
    vertices = []
    faces = []

    # Helper function to add a vertex and return its 1-based index
    def add_vertex(x, y, z):
        vertices.append((x, y, z))
        return len(vertices)

    # --------------------------------------------------
    # 1. Generate trunk vertices and faces
    # --------------------------------------------------
    # We'll build a cylinder with 'slices' subdivisions
    # bottom center at y=0, top center at y=trunk_height
    bottom_center_idx = add_vertex(0.0, 0.0, 0.0)
    top_center_idx    = add_vertex(0.0, trunk_height, 0.0)

    # Make circle vertices
    bottom_circle = []
    top_circle    = []
    for i in range(slices):
        angle = 2.0 * math.pi * i / slices
        x = trunk_radius * math.cos(angle)
        z = trunk_radius * math.sin(angle)
        bottom_circle.append(add_vertex(x, 0.0, z))
        top_circle.append(add_vertex(x, trunk_height, z))

    # -- Triangulate bottom (fan) --
    for i in range(slices):
        i_next = (i + 1) % slices
        faces.append((bottom_center_idx, bottom_circle[i], bottom_circle[i_next]))

    # -- Triangulate top (fan) --
    for i in range(slices):
        i_next = (i + 1) % slices
        faces.append((top_center_idx, top_circle[i_next], top_circle[i]))

    # -- Side of cylinder --
    for i in range(slices):
        i_next = (i + 1) % slices
        b1 = bottom_circle[i]
        b2 = bottom_circle[i_next]
        t1 = top_circle[i]
        t2 = top_circle[i_next]
        # two triangles to form a quad
        faces.append((b1, b2, t2))
        faces.append((b1, t2, t1))

    # --------------------------------------------------
    # 2. Generate the conical part (the "tree top")
    # --------------------------------------------------
    cone_base_center = add_vertex(0.0, trunk_height, 0.0)
    apex_idx         = add_vertex(0.0, trunk_height + tree_height, 0.0)

    cone_circle = []
    for i in range(slices):
        angle = 2.0 * math.pi * i / slices
        x = tree_base_radius * math.cos(angle)
        z = tree_base_radius * math.sin(angle)
        cone_circle.append(add_vertex(x, trunk_height, z))

    # -- Triangulate cone base --
    for i in range(slices):
        i_next = (i + 1) % slices
        faces.append((cone_base_center, cone_circle[i], cone_circle[i_next]))

    # -- Triangulate cone sides --
    for i in range(slices):
        i_next = (i + 1) % slices
        faces.append((apex_idx, cone_circle[i_next], cone_circle[i]))

    return vertices, faces


def compute_face_normal(p1, p2, p3):
    """
    Compute a normalized face normal using cross product of (p2-p1) x (p3-p1).
    p1, p2, p3 are (x, y, z) tuples.
    Returns (nx, ny, nz).
    """
    import math
    # Vector edges
    v1 = (p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2])
    v2 = (p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2])
    # Cross product
    cx = v1[1]*v2[2] - v1[2]*v2[1]
    cy = v1[2]*v2[0] - v1[0]*v2[2]
    cz = v1[0]*v2[1] - v1[1]*v2[0]
    # Normalize
    length = math.sqrt(cx*cx + cy*cy + cz*cz)
    if length < 1e-9:
        # Avoid division by zero, return something
        return (0.0, 1.0, 0.0)
    return (cx/length, cy/length, cz/length)


def expand_to_triangle_list(vertices, faces):
    """
    Given a list of unique vertices and faces as index triplets (1-based),
    produce a "triangle list" (each face expanded to 3 separate vertices).

    Returns:
        tri_vertices: list of (x, y, z) for each face's vertices
        tri_uvs:      list of (u, v) for each face's vertices
        tri_normals:  list of (nx, ny, nz) for each face's vertices
    """
    tri_vertices = []
    tri_uvs      = []
    tri_normals  = []

    # A simple UV pattern per face: (0,0), (1,0), (0.5,1)
    uv_pattern = [(0.0, 0.0), (1.0, 0.0), (0.5, 1.0)]

    for (i1, i2, i3) in faces:
        # Convert from 1-based to 0-based
        p1 = vertices[i1 - 1]
        p2 = vertices[i2 - 1]
        p3 = vertices[i3 - 1]

        # Compute face normal
        n = compute_face_normal(p1, p2, p3)

        # For each vertex in the face, duplicate the position, normal, uv
        tri_vertices.append(p1)
        tri_vertices.append(p2)
        tri_vertices.append(p3)

        tri_normals.append(n)
        tri_normals.append(n)
        tri_normals.append(n)

        # Hardcode the UV pattern
        tri_uvs.append(uv_pattern[0])
        tri_uvs.append(uv_pattern[1])
        tri_uvs.append(uv_pattern[2])

    return tri_vertices, tri_uvs, tri_normals


def print_in_custom_format(tri_vertices, tri_uvs, tri_normals):
    """
    Print the data in the custom format you specified:
      tri 0 0
      vert N
      (x y z)  # repeated N times
      tex N
      (u v)    # repeated N times
      norm N
      (nx ny nz)  # repeated N times
    """
    # The number of expanded vertices
    N = len(tri_vertices)

    # 1) Print "tri 0 0"
    sys.stdout.write("tri 0 0\n")

    # 2) Print vertex count and each vertex
    sys.stdout.write(f"vert {N}\n")
    for (x, y, z) in tri_vertices:
        sys.stdout.write(f"{x:.4f} {y:.4f} {z:.4f}\n")

    # 3) Print texture coordinates
    sys.stdout.write(f"tex {N}\n")
    for (u, v) in tri_uvs:
        sys.stdout.write(f"{u:.4f} {v:.4f}\n")

    # 4) Print normals
    sys.stdout.write(f"norm {N}\n")
    for (nx, ny, nz) in tri_normals:
        sys.stdout.write(f"{nx:.4f} {ny:.4f} {nz:.4f}\n")


if __name__ == "__main__":
    # 1) Generate a simplified trunk+cone Christmas tree
    vertices, faces = generate_christmas_tree_simplified(
        trunk_radius=0.1,    # adjust as needed
        trunk_height=0.3,    # adjust as needed
        tree_base_radius=0.5,# adjust as needed
        tree_height=1.0,     # adjust as needed
        slices=4             # fewer slices => simpler geometry
    )

    # 2) Convert from shared vertex+face lists to a full triangle list
    tri_vertices, tri_uvs, tri_normals = expand_to_triangle_list(vertices, faces)

    # 3) Print the final data in the requested custom format
    print_in_custom_format(tri_vertices, tri_uvs, tri_normals)
