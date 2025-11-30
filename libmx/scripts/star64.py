import math

def cross_product(a, b):
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )

def normalize(v):
    length = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if length < 1e-9:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def generate_mario64_star_mxmod_format(
    scale=1.0, 
    num_points=5,
    outer_radius_factor=1.0,
    inner_radius_factor=0.5,
    z_height_factor=0.5 # Controls how "thick" the star is along the Z-axis
):
    """
    Generate an INVERTED Mario 64-style 3D star mesh in mxmod format.
    The inversion allows the camera to be inside the star and view the faces.
    
    :param scale:           Overall size multiplier for the star.
    :param num_points:      Number of major points (spikes) in the star (e.g., 5 for a classic star).
    :param outer_radius_factor: Multiplier for the radius of the outer spike points.
    :param inner_radius_factor: Multiplier for the radius of the inner points between spikes.
    :param z_height_factor: Multiplier for the star's thickness along the Z-axis.
    :return: A string containing the inverted model data in mxmod format.
    """
    
    # --- OBSOLETE/REDUNDANT VERTEX SETUP (kept for reference, not used by final geometry block) ---
    vertices = []
    
    # 1. Generate the core (center) vertices
    center_top = (0.0, 0.0, z_height_factor * scale)
    center_bottom = (0.0, 0.0, -z_height_factor * scale)
    
    # 2. Generate the ring vertices (outer spikes and inner points)
    outer_ring_vertices = [] # Store indices for easy connection
    inner_ring_vertices = [] # Store indices for easy connection

    angle_step = 2.0 * math.pi / num_points # Angle between major spikes
    half_angle_step = angle_step / 2.0      # Angle to inner points

    for i in range(num_points):
        # Outer spike point (higher radius)
        outer_angle = angle_step * i
        ox = outer_radius_factor * scale * math.cos(outer_angle)
        oy = outer_radius_factor * scale * math.sin(outer_angle)
        outer_ring_vertices.append((ox, oy, 0.0)) # All ring points start at Z=0

        # Inner point (lower radius, halfway between spikes)
        inner_angle = angle_step * i + half_angle_step
        ix = inner_radius_factor * scale * math.cos(inner_angle)
        iy = inner_radius_factor * scale * math.sin(inner_angle)
        inner_ring_vertices.append((ix, iy, 0.0))

    # Add all unique vertices to the main list
    # The order here is important for indexing later
    vertices.append(center_top) # V0
    vertices.append(center_bottom) # V1
    
    start_outer_idx = len(vertices)
    for v in outer_ring_vertices:
        vertices.append(v)
    
    start_inner_idx = len(vertices)
    for v in inner_ring_vertices:
        vertices.append(v)

    # Helper to get vertex by type and index (FIXED: idx is now optional)
    def get_vertex(v_type, idx=None):
        if v_type == 'outer': return vertices[start_outer_idx + idx]
        if v_type == 'inner': return vertices[start_inner_idx + idx]
        if v_type == 'top': return vertices[0]
        if v_type == 'bottom': return vertices[1]
        return None # Should not happen

    # --- OBSOLETE TRIANGLE GENERATION LOGIC (NOW COMMENTED OUT) ---
    # This entire block caused the TypeError and has been commented out because 
    # the intended "puffy" star generation starts below.
    """
    # 3) Build the triangles for the star (inverted winding)
    triangles = [] # Each entry is a list of 3 vertex tuples

    for i in range(num_points):
        next_i = (i + 1) % num_points

        # --- Top Faces ---
        # 1. Triangle: center_top, outer_point[i], inner_point[i]
        #    Inverted: inner_point[i], outer_point[i], center_top
        triangles.append([
            get_vertex('inner', i), 
            get_vertex('outer', i), 
            get_vertex('top')
        ])
        
        # 2. Triangle: center_top, inner_point[i], outer_point[next_i]
        #    Inverted: outer_point[next_i], inner_point[i], center_top
        triangles.append([
            get_vertex('outer', next_i), 
            get_vertex('inner', i), 
            get_vertex('top')
        ])

        # --- Bottom Faces ---
        # 3. Triangle: center_bottom, inner_point[i], outer_point[i]
        #    Inverted: outer_point[i], inner_point[i], center_bottom
        triangles.append([
            get_vertex('outer', i), 
            get_vertex('inner', i), 
            get_vertex('bottom')
        ])
        
        # 4. Triangle: center_bottom, outer_point[next_i], inner_point[i]
        #    Inverted: inner_point[i], outer_point[next_i], center_bottom
        triangles.append([
            get_vertex('inner', i), 
            get_vertex('outer', next_i), 
            get_vertex('bottom')
        ])
        
        # --- Side Faces (connecting top and bottom edges) ---
        # Quad between inner[i] and outer[i] (vertical side)
        # Inverted Tri 1: outer[i] (z=0), inner[i] (z=0), center_bottom
        triangles.append([
            get_vertex('inner', i), get_vertex('outer', i), get_vertex('bottom') # Inner-Outer-Bottom
        ])
        # Inverted Tri 2: outer[i] (z=0), inner[i] (z=0), center_top
        triangles.append([
            get_vertex('outer', i), get_vertex('inner', i), get_vertex('top') # Outer-Inner-Top
        ])
        
        # Quad between inner[i] and outer[next_i] (vertical side)
        # Inverted Tri 1: outer[next_i] (z=0), inner[i] (z=0), center_bottom
        triangles.append([
            get_vertex('inner', i), get_vertex('outer', next_i), get_vertex('bottom') # Inner-OuterNext-Bottom
        ])
        # Inverted Tri 2: inner[i] (z=0), outer[next_i] (z=0), center_top
        triangles.append([
            get_vertex('outer', next_i), get_vertex('inner', i), get_vertex('top') # OuterNext-Inner-Top
        ])
    """

    # --- FINAL, WORKING "PUFFY" STAR GEOMETRY GENERATION ---

    # Apexes (these are the centers of the top/bottom faces)
    v_top_center = (0.0, 0.0, z_height_factor * scale)
    v_bottom_center = (0.0, 0.0, -z_height_factor * scale)
    
    # Ring vertices for top and bottom "halves"
    top_outer_points = []
    top_inner_points = []
    bottom_outer_points = []
    bottom_inner_points = []

    for i in range(num_points):
        # Outer spike point for TOP half
        outer_angle = angle_step * i
        ox = outer_radius_factor * scale * math.cos(outer_angle)
        oy = outer_radius_factor * scale * math.sin(outer_angle)
        # The z-value is slightly below the center apex to create a slope
        top_outer_points.append((ox, oy, z_height_factor * scale * 0.7)) 

        # Inner point for TOP half
        inner_angle = angle_step * i + half_angle_step
        ix = inner_radius_factor * scale * math.cos(inner_angle)
        iy = inner_radius_factor * scale * math.sin(inner_angle)
        # The z-value is slightly above the mid-plane to create a rounded valley
        top_inner_points.append((ix, iy, z_height_factor * scale * 0.3)) 

        # Outer spike point for BOTTOM half
        bottom_outer_points.append((ox, oy, -z_height_factor * scale * 0.7))

        # Inner point for BOTTOM half
        bottom_inner_points.append((ix, iy, -z_height_factor * scale * 0.3))

    # All final triangles
    all_final_triangles = [] # Each element is a list of 3 vertex tuples

    # --- TOP CAP FACES (around v_top_center) ---
    for i in range(num_points):
        next_i = (i + 1) % num_points
        
        # Face: v_top_center, top_outer[i], top_inner[i] -> INVERTED
        all_final_triangles.append([
            top_inner_points[i], top_outer_points[i], v_top_center
        ])
        
        # Face: v_top_center, top_inner[i], top_outer[next_i] -> INVERTED
        all_final_triangles.append([
            top_outer_points[next_i], top_inner_points[i], v_top_center
        ])

    # --- BOTTOM CAP FACES (around v_bottom_center) ---
    for i in range(num_points):
        next_i = (i + 1) % num_points
        
        # Face: v_bottom_center, bottom_inner[i], bottom_outer[i] -> INVERTED
        all_final_triangles.append([
            bottom_outer_points[i], bottom_inner_points[i], v_bottom_center
        ])
        
        # Face: v_bottom_center, bottom_outer[next_i], bottom_inner[i] -> INVERTED
        all_final_triangles.append([
            bottom_inner_points[i], bottom_outer_points[next_i], v_bottom_center
        ])
            
    # --- SIDE FACES (connecting top and bottom rings) ---
    for i in range(num_points):
        next_i = (i + 1) % num_points
        
        # Sides of the "spikes" (forming the sloped edge of a star point)
        # INVERTED Tri 1: inner, top_inner, top_outer
        all_final_triangles.append([
            bottom_inner_points[i], top_inner_points[i], top_outer_points[i]
        ])
        
        # INVERTED Tri 2: top_outer, bottom_outer, bottom_inner
        all_final_triangles.append([
            top_outer_points[i], bottom_outer_points[i], bottom_inner_points[i]
        ])

        # Sides of the "valleys" (forming the sloped edge between points)
        # INVERTED Tri 3: bottom_outer[next], top_outer[next], top_inner
        all_final_triangles.append([
            bottom_outer_points[next_i], top_outer_points[next_i], top_inner_points[i]
        ])

        # INVERTED Tri 4: top_inner, bottom_inner, bottom_outer[next]
        all_final_triangles.append([
            top_inner_points[i], bottom_inner_points[i], bottom_outer_points[next_i]
        ])

    # Flatten out all triangles into the final mxmod data
    all_verts = []
    all_norms = []
    all_tex = []

    # Simple sequential UVs for each triangle face
    uv_set_1 = [(0.0, 0.0), (1.0, 0.0), (0.5, 1.0)] 
    uv_set_2 = [(0.0, 1.0), (1.0, 1.0), (0.5, 0.0)] 

    for idx, tri in enumerate(all_final_triangles):
        vA, vB, vC = tri

        # Calculate the normal for the INVERTED face 
        vec_AB = (vB[0] - vA[0], vB[1] - vA[1], vB[2] - vA[2])
        vec_AC = (vC[0] - vA[0], vC[1] - vA[1], vC[2] - vA[2])
        
        face_norm = normalize(cross_product(vec_AB, vec_AC))

        # Assign vertices, normals, and texture coordinates
        for i, v in enumerate(tri):
            all_verts.append(v)
            all_norms.append(face_norm)
            
            if idx % 2 == 0:
                all_tex.append(uv_set_1[i])
            else:
                all_tex.append(uv_set_2[i])

    # Convert to mxmod-like text
    output_lines = []
    output_lines.append("tri 0 0")

    # Vertices
    output_lines.append(f"vert {len(all_verts)}")
    for vx, vy, vz in all_verts:
        output_lines.append(f"{vx:.6f} {vy:.6f} {vz:.6f}")

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
    # --- SKYBOX SCALE PARAMETERS ---
    # These values make the star incredibly large to act as a skybox
    SKYBOX_SCALE = 20000.0 # Even larger scale for a truly encompassing feel
    
    mxmod_data = generate_mario64_star_mxmod_format(
        scale=SKYBOX_SCALE,
        num_points=5,
        outer_radius_factor=1.0,  # Main points of the star
        inner_radius_factor=0.4,  # Inner points, making the valleys deeper
        z_height_factor=0.6       # Overall "thickness" of the star
    )

    OUTPUT_FILENAME = "mario64_star_skybox.mxmod"

    print(f"Generating large, inverted Mario 64-style star mesh into {OUTPUT_FILENAME}...")
    
    try:
        with open(OUTPUT_FILENAME, 'w') as f:
            f.write(mxmod_data)
        print(f"Successfully created {OUTPUT_FILENAME}.")
        print("This file contains an inverted Mario 64-style star mesh, ready to be used as an inner skybox.")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    main()