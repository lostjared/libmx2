#!/usr/bin/env python3
import math

def write_block(f, vertices, tex_coords, normals):
    """
    Write one MXMOD block.
    Format:
      tri 0 0
      vert <num_vertices>
      (each vertex: x y z)
      tex <num_texture_coords>
      (each texture coordinate: u v)
      norm <num_normals>
      (each normal: nx ny nz)
    """
    f.write("tri 0 0\n")
    f.write("vert {}\n".format(len(vertices)))
    for v in vertices:
        f.write("{:.6f} {:.6f} {:.6f}\n".format(*v))
    f.write("tex {}\n".format(len(tex_coords)))
    for t in tex_coords:
        f.write("{:.6f} {:.6f}\n".format(*t))
    f.write("norm {}\n".format(len(normals)))
    for n in normals:
        f.write("{:.6f} {:.6f} {:.6f}\n".format(*n))

def generate_cube_faces():
    """
    Generate the five faces of a cube (bottom and four sides)
    with extents from -1 to 1. The top face is omitted because it
    will be replaced with a peaked roof.
    Each face is built as two triangles.
    Texture coordinates are mapped from the vertex positions.
    Normals are set to face inward.
    """
    faces = []
    
    # Bottom face (z = -1), seen from inside the cube the interior is upward.
    bottom_vertices = [
        (-1, -1, -1),
        ( 1, -1, -1),
        ( 1,  1, -1),
        (-1,  1, -1)
    ]
    # For inside view, we want the triangle winding to be clockwise when viewed from inside.
    bottom_verts = [bottom_vertices[0], bottom_vertices[1], bottom_vertices[2],
                    bottom_vertices[0], bottom_vertices[2], bottom_vertices[3]]
    def tex_bottom(v):
        x, y, _ = v
        return ((x + 1) / 2, (y + 1) / 2)
    bottom_tex = [tex_bottom(v) for v in bottom_verts]
    # Interior bottom: normal points inward (downward), i.e. (0,0,-1)
    bottom_norm = [(0, 0, -1)] * len(bottom_verts)
    faces.append((bottom_verts, bottom_tex, bottom_norm))
    
    # Front face (y = 1)
    front_vertices = [
        (-1,  1, -1),
        ( 1,  1, -1),
        ( 1,  1,  1),
        (-1,  1,  1)
    ]
    front_verts = [front_vertices[0], front_vertices[1], front_vertices[2],
                   front_vertices[0], front_vertices[2], front_vertices[3]]
    def tex_front(v):
        x, _, z = v
        return ((x + 1) / 2, (z + 1) / 2)
    front_tex = [tex_front(v) for v in front_verts]
    # From inside the cube, front face normal points inward: (0,-1,0)
    front_norm = [(0, -1, 0)] * len(front_verts)
    faces.append((front_verts, front_tex, front_norm))
    
    # Back face (y = -1)
    back_vertices = [
        ( 1, -1, -1),
        (-1, -1, -1),
        (-1, -1,  1),
        ( 1, -1,  1)
    ]
    back_verts = [back_vertices[0], back_vertices[1], back_vertices[2],
                  back_vertices[0], back_vertices[2], back_vertices[3]]
    def tex_back(v):
        x, _, z = v
        return ((x + 1) / 2, (z + 1) / 2)
    back_tex = [tex_back(v) for v in back_verts]
    # Inside back face normal: (0, 1, 0)
    back_norm = [(0, 1, 0)] * len(back_verts)
    faces.append((back_verts, back_tex, back_norm))
    
    # Left face (x = -1)
    left_vertices = [
        (-1, -1, -1),
        (-1,  1, -1),
        (-1,  1,  1),
        (-1, -1,  1)
    ]
    left_verts = [left_vertices[0], left_vertices[1], left_vertices[2],
                  left_vertices[0], left_vertices[2], left_vertices[3]]
    def tex_left(v):
        _, y, z = v
        return ((y + 1) / 2, (z + 1) / 2)
    left_tex = [tex_left(v) for v in left_verts]
    # Inside left face normal: (1, 0, 0) (pointing inward)
    left_norm = [(1, 0, 0)] * len(left_verts)
    faces.append((left_verts, left_tex, left_norm))
    
    # Right face (x = 1)
    right_vertices = [
        ( 1,  1, -1),
        ( 1, -1, -1),
        ( 1, -1,  1),
        ( 1,  1,  1)
    ]
    right_verts = [right_vertices[0], right_vertices[1], right_vertices[2],
                   right_vertices[0], right_vertices[2], right_vertices[3]]
    def tex_right(v):
        _, y, z = v
        return ((y + 1) / 2, (z + 1) / 2)
    right_tex = [tex_right(v) for v in right_verts]
    # Inside right face normal: (-1, 0, 0)
    right_norm = [(-1, 0, 0)] * len(right_verts)
    faces.append((right_verts, right_tex, right_norm))
    
    return faces

def compute_normal(v0, v1, v2):
    """
    Compute the normal for a triangle defined by vertices v0, v1, v2.
    Returns a normalized vector.
    """
    ax, ay, az = v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]
    bx, by, bz = v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]
    # Cross product
    nx = ay * bz - az * by
    ny = az * bx - ax * bz
    nz = ax * by - ay * bx
    length = math.sqrt(nx*nx + ny*ny + nz*nz)
    if length == 0:
        return (0, 0, 0)
    return (nx/length, ny/length, nz/length)

def orient_normal_inward(v0, v1, v2, normal):
    """
    For the top roof triangles the inside of the skybox is below.
    If the computed normal’s z-component is positive (pointing upward),
    flip it so that it points inward (downward).
    """
    if normal[2] > 0:
        return (-normal[0], -normal[1], -normal[2])
    return normal

def generate_top_pyramid(peak_height=2.0):
    """
    Generate a pyramid that replaces the top of the cube.
    The base of the pyramid is the same as the cube top (a square with corners at (-1,-1,1),
    (1,-1,1), (1,1,1), and (-1,1,1)). The peak is at (0,0,peak_height).
    The roof is built as four triangles (one per edge) with texture coordinates
    mapped from the base (and the peak mapped to (0.5,0.5)).
    Normals are computed per triangle and flipped (if necessary) so that the interior of the skybox is lit.
    """
    # Base corners of the top (cube top)
    v0 = (-1, -1, 1)
    v1 = ( 1, -1, 1)
    v2 = ( 1,  1, 1)
    v3 = (-1,  1, 1)
    peak = (0, 0, peak_height)
    
    # We'll create four triangles: one per edge.
    vertices = []
    tex_coords = []
    normals = []
    
    # Helper to map a vertex's (x,y) to texture coordinate [0,1]
    def map_tex(v):
        x, y, _ = v
        return ((x + 1) / 2, (y + 1) / 2)
    
    # Define triangles for each edge.
    # For proper closure when viewed from inside, we choose the vertex order
    # so that the computed normal (via cross product) points downward.
    tri_defs = [
        (v0, v1, peak),
        (v1, v2, peak),
        (v2, v3, peak),
        (v3, v0, peak)
    ]
    
    for a, b, c in tri_defs:
        # Compute normal using the cross product; then flip if necessary.
        n = compute_normal(a, b, c)
        n = orient_normal_inward(a, b, c, n)
        # Append triangle vertices.
        vertices.extend([a, b, c])
        # For texture, map base vertices normally; assign (0.5,0.5) to the peak.
        tex_coords.extend([map_tex(a), map_tex(b), (0.5, 0.5)])
        normals.extend([n, n, n])
    
    return vertices, tex_coords, normals

def main():
    output_filename = "skybox_pyramid_top.mxmod"
    with open(output_filename, "w") as f:
        # Write the cube's bottom and side faces.
        cube_faces = generate_cube_faces()
        for verts, tex, norms in cube_faces:
            write_block(f, verts, tex, norms)
        # Write the pyramid top (roof).
        top_verts, top_tex, top_norms = generate_top_pyramid(peak_height=2.0)
        write_block(f, top_verts, top_tex, top_norms)
    print("MXMOD file '{}' generated successfully.".format(output_filename))

if __name__ == "__main__":
    main()
