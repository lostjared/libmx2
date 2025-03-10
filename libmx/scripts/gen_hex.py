#!/usr/bin/env python3
import math

def write_block(f, vertices, tex_coords, normals):
    """
    Write one MXMOD block.
    
    The format is:
      tri 0 0
      vert <num_vertices>
      [each vertex: x y z per line]
      tex <num_texture_coords>
      [each texture coordinate: u v per line]
      norm <num_normals>
      [each normal: nx ny nz per line]
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
    Generate the 5 faces of a cube (bottom, front, back, left, right)
    using extents from -1 to 1. (We omit the top because that will be our hexagon.)
    Each face is defined as two triangles.
    Texture coordinates are simply a remap of the appropriate vertex components.
    """
    faces = []
    
    # Bottom face (z = -1)
    bottom_vertices = [
        (-1, -1, -1),
        ( 1, -1, -1),
        ( 1,  1, -1),
        (-1,  1, -1)
    ]
    # Two triangles: v0,v1,v2 and v0,v2,v3.
    bottom_verts = [bottom_vertices[0], bottom_vertices[1], bottom_vertices[2],
                    bottom_vertices[0], bottom_vertices[2], bottom_vertices[3]]
    # For bottom face, use x and y to generate texture coordinates:
    def tex_bottom(v):
        x, y, _ = v
        return ((x + 1) / 2, (y + 1) / 2)
    bottom_tex = [tex_bottom(v) for v in bottom_verts]
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
    # Map x and z: x from -1 to 1 becomes u, z from -1 to 1 becomes v.
    def tex_front(v):
        x, _, z = v
        return ((x + 1) / 2, (z + 1) / 2)
    front_tex = [tex_front(v) for v in front_verts]
    front_norm = [(0, 1, 0)] * len(front_verts)
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
    back_norm = [(0, -1, 0)] * len(back_verts)
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
    # Use y and z for texture coordinates.
    def tex_left(v):
        _, y, z = v
        return ((y + 1) / 2, (z + 1) / 2)
    left_tex = [tex_left(v) for v in left_verts]
    left_norm = [(-1, 0, 0)] * len(left_verts)
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
    right_norm = [(1, 0, 0)] * len(right_verts)
    faces.append((right_verts, right_tex, right_norm))
    
    return faces

def generate_top_hexagon():
    """
    Generate the top face as a hexagon (instead of the standard square).
    The hexagon is defined as a fan with its center at (0,0,1) and six points on a circle
    (radius = 1). Texture coordinates are computed by remapping (x,y) from [-1,1] to [0,1].
    Normals are all (0,0,1) since the face is horizontal.
    """
    center = (0.0, 0.0, 1.0)
    num_points = 6
    hex_points = []
    for i in range(num_points):
        angle = math.radians(60 * i)
        x = math.cos(angle)
        y = math.sin(angle)
        hex_points.append((x, y, 1.0))
        
    verts = []
    tex = []
    norms = []
    # Build a triangle fan: for each edge from hex_points[i] to hex_points[(i+1)%num_points]
    for i in range(num_points):
        v1 = center
        v2 = hex_points[i]
        v3 = hex_points[(i + 1) % num_points]
        verts.extend([v1, v2, v3])
        # Texture coordinates: map x and y from [-1,1] to [0,1]
        def map_tex(v):
            x, y, _ = v
            return ((x + 1) / 2, (y + 1) / 2)
        tex.extend([map_tex(v1), map_tex(v2), map_tex(v3)])
        norms.extend([(0, 0, 1)] * 3)
    return verts, tex, norms

def main():
    output_filename = "skybox_hex_top.mxmod"
    with open(output_filename, "w") as f:
        # Write the bottom and four side faces of the cube.
        cube_faces = generate_cube_faces()
        for verts, tex, norms in cube_faces:
            write_block(f, verts, tex, norms)
        # Write the top face as a hexagon.
        top_verts, top_tex, top_norms = generate_top_hexagon()
        write_block(f, top_verts, top_tex, top_norms)
    print("MXMOD file '{}' generated successfully.".format(output_filename))

if __name__ == "__main__":
    main()
