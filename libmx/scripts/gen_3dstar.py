import numpy as np

# --- Configuration ---
SKYBOX_SIZE = 500.0  # Defines the half-edge length of the cube (e.g., cube is 1000x1000x1000)
OUTPUT_FILENAME = "inverted_skybox_cube.mxmod"

# The header for the file, indicating a single triangle list (tri 0 0)
HEADER = "tri 0 0\n"

def generate_inverted_cube_data(size):
    """
    Generates vertex, texture, and normal data for an inverted cube.
    The cube faces are defined to point inward.
    """
    
    # 1. Define 8 base vertices of a standard cube centered at the origin
    v = size
    # Vertices (V0 to V7)
    V = [
        (-v, -v, -v), # V0: (-1, -1, -1)
        ( v, -v, -v), # V1: ( 1, -1, -1)
        ( v,  v, -v), # V2: ( 1,  1, -1)
        (-v,  v, -v), # V3: (-1,  1, -1)
        (-v, -v,  v), # V4: (-1, -1,  1)
        ( v, -v,  v), # V5: ( 1, -1,  1)
        ( v,  v,  v), # V6: ( 1,  1,  1)
        (-v,  v,  v)  # V7: (-1,  1,  1)
    ]
    
    # 2. Define Texture Coordinates (T0 to T3)
    # T0: (0.0, 1.0) | T1: (1.0, 1.0)
    # T3: (0.0, 0.0) | T2: (1.0, 0.0)
    T = [
        (0.0, 1.0), # Top-left (TL)
        (1.0, 1.0), # Top-right (TR)
        (1.0, 0.0), # Bottom-right (BR)
        (0.0, 0.0)  # Bottom-left (BL)
    ]

    # 3. Define the faces (12 triangles, 36 vertices total)
    # The order of vertices for each triangle is critical for determining the face normal.
    # To make the cube INVERTED (normals pointing INWARD), we reverse the winding
    # order (e.g., from V1, V2, V3 to V3, V2, V1) compared to a standard cube.
    
    # Face definitions (Vertex Index, Texture Index, Normal Vector)
    faces_data = [] # List of (V_idx, T_idx, N_vec) tuples

    # --- Z-Positive Face (Front) --- Normal: (0, 0, -1)
    # V7, V6, V5 (Inverted winding)
    faces_data.extend([(7, T[0], (0.0, 0.0, -1.0)), (6, T[1], (0.0, 0.0, -1.0)), (5, T[2], (0.0, 0.0, -1.0))]) 
    # V5, V4, V7 (Inverted winding)
    faces_data.extend([(5, T[2], (0.0, 0.0, -1.0)), (4, T[3], (0.0, 0.0, -1.0)), (7, T[0], (0.0, 0.0, -1.0))])

    # --- Z-Negative Face (Back) --- Normal: (0, 0, 1)
    # V0, V1, V2 (Inverted winding)
    faces_data.extend([(0, T[0], (0.0, 0.0, 1.0)), (1, T[1], (0.0, 0.0, 1.0)), (2, T[2], (0.0, 0.0, 1.0))])
    # V2, V3, V0 (Inverted winding)
    faces_data.extend([(2, T[2], (0.0, 0.0, 1.0)), (3, T[3], (0.0, 0.0, 1.0)), (0, T[0], (0.0, 0.0, 1.0))])

    # --- X-Negative Face (Left) --- Normal: (1, 0, 0)
    # V4, V0, V3 (Inverted winding)
    faces_data.extend([(4, T[0], (1.0, 0.0, 0.0)), (0, T[1], (1.0, 0.0, 0.0)), (3, T[2], (1.0, 0.0, 0.0))])
    # V3, V7, V4 (Inverted winding)
    faces_data.extend([(3, T[2], (1.0, 0.0, 0.0)), (7, T[3], (1.0, 0.0, 0.0)), (4, T[0], (1.0, 0.0, 0.0))])
    
    # --- X-Positive Face (Right) --- Normal: (-1, 0, 0)
    # V1, V5, V6 (Inverted winding)
    faces_data.extend([(1, T[0], (-1.0, 0.0, 0.0)), (5, T[1], (-1.0, 0.0, 0.0)), (6, T[2], (-1.0, 0.0, 0.0))])
    # V6, V2, V1 (Inverted winding)
    faces_data.extend([(6, T[2], (-1.0, 0.0, 0.0)), (2, T[3], (-1.0, 0.0, 0.0)), (1, T[0], (-1.0, 0.0, 0.0))])
    
    # --- Y-Negative Face (Bottom) --- Normal: (0, 1, 0)
    # V4, V5, V1 (Inverted winding)
    faces_data.extend([(4, T[0], (0.0, 1.0, 0.0)), (5, T[1], (0.0, 1.0, 0.0)), (1, T[2], (0.0, 1.0, 0.0))])
    # V1, V0, V4 (Inverted winding)
    faces_data.extend([(1, T[2], (0.0, 1.0, 0.0)), (0, T[3], (0.0, 1.0, 0.0)), (4, T[0], (0.0, 1.0, 0.0))])
    
    # --- Y-Positive Face (Top) --- Normal: (0, -1, 0)
    # V3, V2, V6 (Inverted winding)
    faces_data.extend([(3, T[0], (0.0, -1.0, 0.0)), (2, T[1], (0.0, -1.0, 0.0)), (6, T[2], (0.0, -1.0, 0.0))])
    # V6, V7, V3 (Inverted winding)
    faces_data.extend([(6, T[2], (0.0, -1.0, 0.0)), (7, T[3], (0.0, -1.0, 0.0)), (3, T[0], (0.0, -1.0, 0.0))])


    # 4. Format the final output strings
    
    vert_count = len(faces_data) # Should be 36
    
    vert_data = f"vert {vert_count}\n"
    tex_data = f"tex {vert_count}\n"
    norm_data = f"norm {vert_count}\n"

    for v_idx, t_vec, n_vec in faces_data:
        # Vertex position
        vx, vy, vz = V[v_idx]
        vert_data += f"{vx:.6f} {vy:.6f} {vz:.6f}\n"
        
        # Texture coordinate
        tx, ty = t_vec
        tex_data += f"{tx:.1f} {ty:.1f}\n"

        # Normal vector (points inward)
        nx, ny, nz = n_vec
        norm_data += f"{nx:.1f} {ny:.1f} {nz:.1f}\n"
        
    return vert_data, tex_data, norm_data

def create_mxmod_file(filename):
    """Writes the generated mesh data to the .mxmod file."""
    
    print(f"Generating inverted skybox cube mesh (size: {SKYBOX_SIZE} units)...")
    vert_data, tex_data, norm_data = generate_inverted_cube_data(SKYBOX_SIZE)
    
    full_content = HEADER + vert_data + tex_data + norm_data
    
    print(f"Writing data to {filename}...")
    try:
        with open(filename, 'w') as f:
            f.write(full_content)
        print(f"Successfully created {filename} with 36 vertices (12 triangles).")
    except IOError as e:
        print(f"Error writing file: {e}")

# --- Execute Script ---
create_mxmod_file(OUTPUT_FILENAME)