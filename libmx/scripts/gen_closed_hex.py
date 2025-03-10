#!/usr/bin/env python3
import math

def write_block(f, vertices, tex_coords, normals):
    """
    Write one MXMOD block.
    
    Format:
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
    Generate five faces of a cube (bottom, front, back, left, and right) 
    with extents from -1 to 1. (The top is replaced by our hexagon.)
    Each face is defined by two triangles.
    Texture coordinates are simply remapped from the vertex positions.
    """
    faces = []
    
    # Bottom face (z = -1)
    bottom_vertices = [
        (-1, -1, -1),
        ( 1, -1, -1),
        ( 1,  1, -1),
        (-1,  1, -1)
    ]
    bottom_verts = [bottom_vertices[0], bottom_vertices[1], bottom_vertices[2],
                    bottom_vertices[0], bottom_vertices[2], bottom_vertices[3]]
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

def generate_top_hexagon_closed():
    """
    Generate the top face as a closed hexagon.
    
    Instead of a single fan sharing one center vertex, we generate each triangle 
    with its own center vertex (duplicated) so that the seams between triangles do not 
    create any gap when viewed from inside.
    
    The hexagon lies in the plane z = 1.
    We choose a regular hexagon with vertices computed from a circle of radius 1.
    Texture coordinates are mapped from (x,y) in [-1,1] to [0,1] and all normals are (0,0,1).
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
    
    # For each triangle, duplicate the center so that each triangle is independent.
    for i in range(num_points):
        # Duplicate center for this triangle
        c = (center[0], center[1], center[2])
        v1 = hex_points[i]
        v2 = hex_points[(i + 1) % num_points]
        verts.extend([c, v1, v2])
        def map_tex(v):
            x, y, _ = v
            return ((x + 1) / 2, (y + 1) / 2)
        tex.extend([map_tex(c), map_tex(v1), map_tex(v2)])
        norms.extend([(0, 0, 1)] * 3)
    
    return verts, tex, norms

def main():
    output_filename = "skybox_hex_top_closed.mxmod"
    with open(output_filename, "w") as f:
        # Write the bottom and side faces of the cube.
        cube_faces = generate_cube_faces()
        for verts, tex, norms in cube_faces:
            write_block(f, verts, tex, norms)
        # Write the top face as a closed hexagon.
        top_verts, top_tex, top_norms = generate_top_hexagon_closed()
        write_block(f, top_verts, top_tex, top_norms)
    print("MXMOD file '{}' generated successfully.".format(output_filename))

if __name__ == "__main__":
    main()
