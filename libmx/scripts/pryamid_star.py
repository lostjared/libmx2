import math

def generate_star_mxmod(filename="3d_star.mxmod", radius_outer=1.0, height=1.5):
    """
    Generates an .mxmod file for a Pentagonal Bipyramid, a star-like shape.

    - The base is a pentagon in the X-Y plane.
    - It has a point (apex) above and below the base.
    - Total 10 faces (triangles), 30 vertices, 30 UVs, 30 Normals.
    """

    # --- 1. Calculate Base Pentagon Vertices (5 points in a circle) ---
    vertices_base = []
    angle_step = 2 * math.pi / 5
    for i in range(5):
        angle = i * angle_step
        x = radius_outer * math.cos(angle)
        y = radius_outer * math.sin(angle)
        vertices_base.append((x, y, 0.0))

    # Define the two apex points
    apex_top = (0.0, 0.0, height / 2.0)
    apex_bottom = (0.0, 0.0, -height / 2.0)

    # --- 2. Construct Mesh Data (30 Vertices, UVs, Normals) ---
    vertices = []
    tex_coords = []
    normals = []
    
    # Simple, non-repeating UVs that cycle through 0.0/1.0 corners
    tex_coords_face = [
        (0.5, 1.0),  # Apex/Center (Approximate)
        (0.0, 0.0),  # Base point 1
        (1.0, 0.0)   # Base point 2
    ]
    
    # Total number of triangles (faces) is 10
    num_triangles = 10 

    for i in range(5):
        # Indices for the base vertices
        v_curr = i
        v_next = (i + 1) % 5
        
        # --- Top Faces (5 triangles) ---
        # Vertices: [Apex_Top, Base[v_next], Base[v_curr]]
        # Order is chosen for counter-clockwise winding (outward-facing)
        face_vertices_top = [apex_top, vertices_base[v_next], vertices_base[v_curr]]
        vertices.extend(face_vertices_top)
        tex_coords.extend(tex_coords_face)
        
        # Calculate the normal for the top face
        # We need to compute the cross product of two edge vectors
        V1 = [face_vertices_top[1][j] - face_vertices_top[0][j] for j in range(3)] # V_next - Apex
        V2 = [face_vertices_top[2][j] - face_vertices_top[0][j] for j in range(3)] # V_curr - Apex
        
        # Cross product (V1 x V2) for the normal
        N = [
            V1[1] * V2[2] - V1[2] * V2[1],
            V1[2] * V2[0] - V1[0] * V2[2],
            V1[0] * V2[1] - V1[1] * V2[0]
        ]
        # Normalize the normal vector
        mag = math.sqrt(N[0]**2 + N[1]**2 + N[2]**2)
        normal_top = (N[0] / mag, N[1] / mag, N[2] / mag)
        normals.extend([normal_top] * 3) # Use the same normal for all 3 vertices (flat shading)
        

        # --- Bottom Faces (5 triangles) ---
        # Vertices: [Apex_Bottom, Base[v_curr], Base[v_next]]
        # Order is chosen for counter-clockwise winding (outward-facing)
        face_vertices_bottom = [apex_bottom, vertices_base[v_curr], vertices_base[v_next]]
        vertices.extend(face_vertices_bottom)
        tex_coords.extend(tex_coords_face)

        # Calculate the normal for the bottom face
        # V1 = V_curr - Apex_bottom, V2 = V_next - Apex_bottom
        V1 = [face_vertices_bottom[1][j] - face_vertices_bottom[0][j] for j in range(3)]
        V2 = [face_vertices_bottom[2][j] - face_vertices_bottom[0][j] for j in range(3)]
        
        # Cross product (V1 x V2)
        N = [
            V1[1] * V2[2] - V1[2] * V2[1],
            V1[2] * V2[0] - V1[0] * V2[2],
            V1[0] * V2[1] - V1[1] * V2[0]
        ]
        mag = math.sqrt(N[0]**2 + N[1]**2 + N[2]**2)
        normal_bottom = (N[0] / mag, N[1] / mag, N[2] / mag)
        normals.extend([normal_bottom] * 3) # Use the same normal for all 3 vertices (flat shading)


    # --- 3. Write to File ---
    with open(filename, 'w') as f:
        # Header/Mesh Info
        f.write("tri 0 0\n")

        # Vertices Section
        f.write(f"vert {len(vertices)}\n")
        for x, y, z in vertices:
            f.write(f"{x} {y} {z}\n")

        # Texture Coordinates Section
        f.write(f"\ntex {len(tex_coords)}\n")
        # UVs are 2D coordinates
        for u, v in tex_coords:
            f.write(f"{u} {v}\n")

        # Normals Section
        f.write(f"\nnorm {len(normals)}\n")
        # Normals are 3D vectors
        for nx, ny, nz in normals:
            f.write(f"{nx} {ny} {nz}\n")

    print(f"Successfully generated {filename} with a {num_triangles}-triangle star mesh.")

# Execute the function to create the file
# Default parameters create a star shape with a 1.0 radius and 1.5 height.
generate_star_mxmod()