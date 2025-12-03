import math

def generate_mxmod_geometry(filename="cigar_tunnel.mxmod", radius=1.0, height=10.0, pyramid_height=2.0, sides=12):
    """
    Generates an MXMOD file for a cigar-shaped tunnel (cylinder with two pyramids).

    Args:
        filename (str): The name of the output MXMOD file.
        radius (float): Radius of the cylinder and the pyramid base.
        height (float): Length of the cylinder body.
        pyramid_height (float): Height of the pyramids.
        sides (int): Number of sides for the cylinder/pyramid base (controls
                     how smooth the "cigar" is). Must be >= 3.
    """

    if sides < 3:
        raise ValueError("Sides must be 3 or more.")

    # --- 1. Generate Vertices and Indices for the Shape ---
    vertices = []
    tex_coords = []
    normals = []

    # Helper for the end coordinates of the cylinder body
    start_z = 0.0
    end_z = height

    # --- Cylinder Body (Open Tube) ---
    # We create a cylinder with two rings of vertices.
    # The sides will be triangles connecting the rings.

    # 1. Base Ring (z = start_z) and Top Ring (z = end_z)
    for i in range(sides):
        angle = 2.0 * math.pi * i / sides
        x = radius * math.cos(angle)
        y = radius * math.sin(angle)

        # Base Ring (inner cylinder end)
        vertices.append((x, y, start_z))
        # Top Ring (outer cylinder end)
        vertices.append((x, y, end_z))

    # 2. Side Faces
    for i in range(sides):
        i1 = 2 * i
        i2 = 2 * ((i + 1) % sides)
        # Vertices are defined by their position in the list
        # V0: (x_i, y_i, start_z)
        # V1: (x_i, y_i, end_z)
        # V2: (x_{i+1}, y_{i+1}, end_z)
        # V3: (x_{i+1}, y_{i+1}, start_z)

        # First Triangle (V0, V1, V2)
        # Second Triangle (V0, V2, V3) - *Corrected to V0, V2, V3 for consistent winding*

        # Triangle 1: (i, end_i, end_{i+1})
        # Triangle 2: (i, end_{i+1}, start_{i+1})

        # Since we stored V_start_z (i1) and V_end_z (i1+1) for each angle,
        # the indices are:
        # V_i_start: i1
        # V_i_end: i1 + 1
        # V_{i+1}_start: i2
        # V_{i+1}_end: i2 + 1

        v0 = vertices[i1]
        v1 = vertices[i1 + 1]
        v2 = vertices[i2 + 1] # V_{i+1}_end
        v3 = vertices[i2]     # V_{i+1}_start

        # --- Face 1 (V0, V1, V2) ---
        # The vertices are added 3 times for each triangle face in MXMOD.
        # This is a common pattern for defining geometry where each face has its own
        # independent texture/normal data.
        vertices.extend([v0, v1, v2])
        # Calculate normal (Should point outwards)
        nx = v0[0] / radius # Approximate normal for a cylinder
        ny = v0[1] / radius
        norm = (nx, ny, 0.0)
        normals.extend([norm, norm, norm])
        # Texture coordinates (U-V mapping)
        u1 = i / sides
        u2 = (i + 1) / sides
        tex_coords.extend([(u1, 0.0), (u1, 1.0), (u2, 1.0)])

        # --- Face 2 (V0, V2, V3) ---
        vertices.extend([v0, v2, v3])
        # Calculate normal
        norm_v3 = (v3[0] / radius, v3[1] / radius, 0.0) # Normal for the V3 side
        normals.extend([norm, norm_v3, norm_v3]) # A mix, keeping it simple for now
        # Texture coordinates
        tex_coords.extend([(u1, 0.0), (u2, 1.0), (u2, 0.0)])

    # Remove the initial "base" vertices as they are not used in the final count,
    # which is based on the number of triangles * 3.
    vertices = vertices[2*sides:]
    total_triangles = len(vertices) // 3


    # --- 2. Pyramids (4-sided for simplicity) ---
    # The sides parameter only controls the cylinder in this simplified script.
    # The pyramid will be a standard 4-sided pyramid.

    pyramid_base_radius = radius
    base_coords = [
        (pyramid_base_radius, 0.0, 0.0),
        (0.0, pyramid_base_radius, 0.0),
        (-pyramid_base_radius, 0.0, 0.0),
        (0.0, -pyramid_base_radius, 0.0)
    ]
    pyramid_sides = 4 # Fixed for simplicity, as per user's prompt

    # --- Pyramid 1 (Start of tunnel, z < start_z) ---
    tip1 = (0.0, 0.0, start_z - pyramid_height)
    
    for i in range(pyramid_sides):
        v_curr = (base_coords[i][0], base_coords[i][1], start_z) # V_i (base)
        v_next = (base_coords[(i + 1) % pyramid_sides][0], base_coords[(i + 1) % pyramid_sides][1], start_z) # V_{i+1} (base)

        # Triangle: Tip, V_next, V_curr
        # This is for the side faces of the pyramid, pointing inward to the tip.
        vertices.extend([tip1, v_next, v_curr])
        # Approximate normal (pointing roughly outward)
        nx = v_curr[0] / radius
        ny = v_curr[1] / radius
        # The Z component needs to be negative since the tip is in -Z
        norm = (nx, ny, -0.5)
        norm = (norm[0] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2),
                norm[1] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2),
                norm[2] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2))
        normals.extend([norm, norm, norm])
        # Texture coordinates
        tex_coords.extend([(0.5, 1.0), (1.0, 0.0), (0.0, 0.0)])
        total_triangles += 1


    # --- Pyramid 2 (End of tunnel, z > end_z) ---
    tip2 = (0.0, 0.0, end_z + pyramid_height)
    
    for i in range(pyramid_sides):
        v_curr = (base_coords[i][0], base_coords[i][1], end_z) # V_i (base)
        v_next = (base_coords[(i + 1) % pyramid_sides][0], base_coords[(i + 1) % pyramid_sides][1], end_z) # V_{i+1} (base)

        # Triangle: Tip, V_curr, V_next
        # This is for the side faces of the pyramid, pointing inward to the tip.
        vertices.extend([tip2, v_curr, v_next])
        # Approximate normal (pointing roughly outward)
        nx = v_curr[0] / radius
        ny = v_curr[1] / radius
        # The Z component needs to be positive since the tip is in +Z
        norm = (nx, ny, 0.5)
        norm = (norm[0] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2),
                norm[1] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2),
                norm[2] / math.sqrt(norm[0]**2 + norm[1]**2 + norm[2]**2))
        normals.extend([norm, norm, norm])
        # Texture coordinates
        tex_coords.extend([(0.5, 1.0), (0.0, 0.0), (1.0, 0.0)])
        total_triangles += 1


    # --- 3. Write the MXMOD File ---
    with open(filename, 'w') as f:
        # Header (Assuming tri 0 0 is a standard starting point)
        f.write("tri 0 0\n")
        f.write(f"vert {total_triangles * 3}\n")

        # Vertices (x y z)
        for v in vertices:
            f.write(f"{v[0]:.3f} {v[1]:.3f} {v[2]:.3f}\n")

        f.write(f"\ntex {total_triangles * 3}\n")
        # Texture Coordinates (u v)
        for t in tex_coords:
            f.write(f"{t[0]:.3f} {t[1]:.3f}\n")

        f.write(f"\nnorm {total_triangles * 3}\n")
        # Normals (nx ny nz)
        for n in normals:
            f.write(f"{n[0]:.3f} {n[1]:.3f} {n[2]:.3f}\n")

    print(f"Successfully generated {filename} with {total_triangles} triangles.")


# --- Configuration and Execution ---
if __name__ == "__main__":
    try:
        # Default parameters create a good-sized shape
        generate_mxmod_geometry(
            filename="cigar_tunnel.mxmod",
            radius=2.0,       # Radius of the tunnel
            height=20.0,      # Length of the main tunnel body
            pyramid_height=5.0, # Length of the pyramid ends
            sides=16          # Number of sides for the cylinder (16 sides = 16 faces * 2 triangles/face)
        )
    except Exception as e:
        print(f"An error occurred: {e}")