import math

def generate_ufo(num_segments=8, saucer_radius=0.4, saucer_height=0.2, dome_radius=0.1, dome_height=0.05):
    """
    Generate a simple UFO shape:
    - A saucer with a top and bottom "cap"
    - A small dome on top
    
    num_segments: number of segments around the saucer ring
    saucer_radius: radius of the saucer
    saucer_height: half-height of the saucer (so total height ~0.4 if top + bottom)
    dome_radius: radius of the top dome
    dome_height: height of the top dome
    
    Coordinates:
    - We'll center the saucer around the origin on x-y plane.
    - The saucer top center will be around z = 0.2
    - The bottom center will be around z = -0.2
    - The dome sits on top center (z ~ 0.2)
    """
    
    vertices = []
    texcoords = []
    normals = []
    indices = []  # Will store indices for triangles
    
    # Helper functions
    def add_vertex(x, y, z):
        vertices.append((x, y, z))
        # Simple texture coordinates based on position (for demonstration)
        # Map x,y to texture space [0,1]
        u = 0.5 + x
        v = 0.5 + y
        texcoords.append((u, v))
        # Simple normal: For a UFO, let's just point all normals upwards for simplicity
        # A more accurate model would compute averaged normals.
        normals.append((0.0, 0.0, 1.0))
        return len(vertices)-1
    
    # Create the saucer top and bottom:
    # Top center vertex:
    top_center = add_vertex(0.0, 0.0, saucer_height)
    
    # Bottom center vertex:
    bottom_center = add_vertex(0.0, 0.0, -saucer_height)
    
    # Create a ring of vertices around at z=0 for the saucer 'equator'
    ring_indices = []
    for i in range(num_segments):
        angle = 2.0 * math.pi * i / num_segments
        x = saucer_radius * math.cos(angle)
        y = saucer_radius * math.sin(angle)
        z = 0.0
        ring_indices.append(add_vertex(x, y, z))
    
    # Triangulate the top part of the saucer (fan from top_center)
    # top_center -> ring vertices
    for i in range(num_segments):
        i1 = ring_indices[i]
        i2 = ring_indices[(i+1) % num_segments]
        # Triangle: top_center, i1, i2
        indices.extend([top_center, i1, i2])
    
    # Triangulate the bottom part of the saucer (fan from bottom_center)
    for i in range(num_segments):
        i1 = ring_indices[i]
        i2 = ring_indices[(i+1) % num_segments]
        # Triangle: bottom_center, i2, i1 (to keep winding consistent)
        indices.extend([bottom_center, i2, i1])
    
    # Create the dome on top (a small hemisphere):
    # We'll make a small circle of vertices plus a top vertex to form a fan.
    dome_top = add_vertex(0.0, 0.0, saucer_height + dome_height)
    dome_ring_indices = []
    for i in range(num_segments):
        angle = 2.0 * math.pi * i / num_segments
        x = dome_radius * math.cos(angle)
        y = dome_radius * math.sin(angle)
        z = saucer_height  # bottom of dome ring sits at top of saucer
        dome_ring_indices.append(add_vertex(x, y, z))
    
    # Triangulate the dome (fan from dome_top)
    for i in range(num_segments):
        i1 = dome_ring_indices[i]
        i2 = dome_ring_indices[(i+1) % num_segments]
        indices.extend([dome_top, i1, i2])
    
    # Now we have all vertices, normals, and indices.
    # The format shown by the user is something like:
    # tri 0 0
    # vert N
    # <N lines of vertices>
    # tex N
    # <N lines of texture coords>
    # norm N
    # <N lines of normals>
    
    # The user's snippet shows a single "tri 0 0" block with a single vertex array.
    # It does not explicitly show indices. The assumption is that the listed vertices, tex, norm
    # lines are in the order they are used for the triangles.
    #
    # In the provided snippet, each set of three vertices forms a triangle directly in order.
    # We'll therefore "expand" indices into a direct triangle vertex listing.
    
    final_verts = []
    final_tex = []
    final_norms = []
    for idx in indices:
        vx, vy, vz = vertices[idx]
        final_verts.append((vx, vy, vz))
        ux, uy = texcoords[idx]
        final_tex.append((ux, uy))
        nx, ny, nz = normals[idx]
        final_norms.append((nx, ny, nz))
    
    # Print out in the requested format:
    print("tri 0 0")
    print(f"vert {len(final_verts)}")
    for v in final_verts:
        print(f"{v[0]} {v[1]} {v[2]}")
    print(f"tex {len(final_tex)}")
    for t in final_tex:
        print(f"{t[0]} {t[1]}")
    print(f"norm {len(final_norms)}")
    for n in final_norms:
        print(f"{n[0]} {n[1]} {n[2]}")


if __name__ == "__main__":
    generate_ufo()
