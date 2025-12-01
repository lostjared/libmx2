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

def generate_inverted_cuboid_segment(x_min, x_max, y_min, y_max, z_min, z_max):
    """
    Generates 12 triangles (6 faces) for a single cuboid segment,
    with all faces inverted (normals point inward) for skybox use.
    
    Returns: list of 3-tuples (vertex_position, tex_coord, normal_vector)
    """
    
    # Vertices (defined by min/max extents)
    V = [
        (x_min, y_min, z_min), # V0
        (x_max, y_min, z_min), # V1
        (x_max, y_max, z_min), # V2
        (x_min, y_max, z_min), # V3
        (x_min, y_min, z_max), # V4
        (x_max, y_min, z_max), # V5
        (x_max, y_max, z_max), # V6
        (x_min, y_max, z_max)  # V7
    ]
    
    # Texture Coordinates (T0 to T3)
    T = [ (0.0, 1.0), (1.0, 1.0), (1.0, 0.0), (0.0, 0.0) ]

    # The list of faces, where each face is defined by 6 indices 
    # (3 for the first triangle, 3 for the second).
    # Normal is the *outward* normal, which we will negate later.
    # Winding is standard (V1, V2, V3) and will be reversed later.
    
    # Face Definition: (Vertex Indices, Outward Normal)
    FACES = [
        # 1. Z-Negative (Back) - Outward Normal (0, 0, -1)
        ([V[0], V[3], V[2], V[0], V[2], V[1]], (0.0, 0.0, -1.0)), 
        # 2. Z-Positive (Front) - Outward Normal (0, 0, 1)
        ([V[4], V[5], V[6], V[4], V[6], V[7]], (0.0, 0.0, 1.0)),   
        # 3. X-Negative (Left) - Outward Normal (-1, 0, 0)
        ([V[0], V[4], V[7], V[0], V[7], V[3]], (-1.0, 0.0, 0.0)),
        # 4. X-Positive (Right) - Outward Normal (1, 0, 0)
        ([V[1], V[2], V[6], V[1], V[6], V[5]], (1.0, 0.0, 0.0)),  
        # 5. Y-Negative (Bottom) - Outward Normal (0, -1, 0)
        ([V[0], V[1], V[5], V[0], V[5], V[4]], (0.0, -1.0, 0.0)),
        # 6. Y-Positive (Top) - Outward Normal (0, 1, 0)
        ([V[3], V[7], V[6], V[3], V[6], V[2]], (0.0, 1.0, 0.0)),
    ]

    triangles_data = [] # List of (V_tuple, T_tuple, N_tuple)
    
    for v_indices, outward_norm in FACES:
        # Determine the inverted normal vector
        inward_norm = (-outward_norm[0], -outward_norm[1], -outward_norm[2])

        # Triangles are defined by (v_indices[i], v_indices[i+1], v_indices[i+2])
        # We need to process them in groups of 3 vertices (2 triangles per face)
        
        # --- Triangle 1 ---
        # Vertices: V(i0), V(i1), V(i2). These are coordinate tuples.
        v0, v1, v2 = v_indices[0], v_indices[1], v_indices[2]
        
        # INVERTED WINDING: (V2, V1, V0)
        # We store the coordinate tuple itself, not the index.
        triangles_data.append((v2, T[2], inward_norm)) # Corner 2 (BR)
        triangles_data.append((v1, T[1], inward_norm)) # Corner 1 (TR)
        triangles_data.append((v0, T[0], inward_norm)) # Corner 0 (TL)

        # --- Triangle 2 ---
        # Vertices: V(i3), V(i4), V(i5). These are coordinate tuples.
        v3, v4, v5 = v_indices[3], v_indices[4], v_indices[5]
        
        # INVERTED WINDING: (V5, V4, V3)
        # We store the coordinate tuple itself, not the index.
        triangles_data.append((v5, T[3], inward_norm)) # Corner 5 (BL)
        triangles_data.append((v4, T[2], inward_norm)) # Corner 4 (BR)
        triangles_data.append((v3, T[0], inward_norm)) # Corner 3 (TL)

    # FIX: triangles_data already contains the (Position, TexCoord, Normal) tuple.
    # We must return it directly, as iterating over it and trying to look up 
    # the coordinate tuple as an index caused the TypeError.
    return triangles_data


def generate_plus_sign_mxmod_format(scale=1000.0):
    """
    Generates the mesh for a 3D plus sign (cross) by combining 6 inverted cuboid segments.
    The final mesh has 72 triangles (216 vertices) and is suitable for a skybox.
    """
    
    # Dimensions (scaled relative to 1.0)
    L = 1.0 * scale  # Total extent of the arm from center
    T = 0.3 * scale  # Half-thickness of the core/arms
    
    # Cuboid Segments needed (6 total arms extending from the core [-T, T])
    segments = []
    
    # 1. +X Arm: (extends from T to L)
    segments.extend(generate_inverted_cuboid_segment(T, L, -T, T, -T, T))
    # 2. -X Arm: (extends from -L to -T)
    segments.extend(generate_inverted_cuboid_segment(-L, -T, -T, T, -T, T))
    
    # 3. +Y Arm: (extends from T to L)
    segments.extend(generate_inverted_cuboid_segment(-T, T, T, L, -T, T))
    # 4. -Y Arm: (extends from -L to -T)
    segments.extend(generate_inverted_cuboid_segment(-T, T, -L, -T, -T, T))

    # 5. +Z Arm: (extends from T to L)
    segments.extend(generate_inverted_cuboid_segment(-T, T, -T, T, T, L))
    # 6. -Z Arm: (extends from -L to -T)
    segments.extend(generate_inverted_cuboid_segment(-T, T, -T, T, -L, -T))

    # 7. Add the central core to close the cavity
    # The core is [-T, T] on all axes.
    segments.extend(generate_inverted_cuboid_segment(-T, T, -T, T, -T, T))

    # --- Flatten data for MXMOD format ---
    all_verts = []
    all_norms = []
    all_tex = []
    
    for v_pos, t_coord, n_vec in segments:
        all_verts.append(v_pos)
        all_tex.append(t_coord)
        # Note: Normals are already inverted by the segment generator
        all_norms.append(n_vec) 

    # Convert to mxmod-like text
    output_lines = []
    output_lines.append("tri 0 0")

    # Vertices
    vert_count = len(all_verts)
    output_lines.append(f"vert {vert_count}")
    for vx, vy, vz in all_verts:
        output_lines.append(f"{vx:.6f} {vy:.6f} {vz:.6f}")

    # Texture coords
    output_lines.append(f"tex {vert_count}")
    for u, v in all_tex:
        output_lines.append(f"{u:.4f} {v:.4f}")

    # Normals
    output_lines.append(f"norm {vert_count}")
    for nx, ny, nz in all_norms:
        output_lines.append(f"{nx:.4f} {ny:.4f} {nz:.4f}")

    return "\n".join(output_lines)

def main():
    # --- SKYBOX SCALE PARAMETERS ---
    # This scale makes the cross 40,000 units wide, ensuring it encompasses the scene.
    SKYBOX_SCALE = 500.0 
    
    mxmod_data = generate_plus_sign_mxmod_format(scale=SKYBOX_SCALE)

    OUTPUT_FILENAME = "plus_sign_skybox.mxmod"

    print(f"Generating large, inverted 3D Plus Sign mesh (7 segments, {len(mxmod_data.splitlines()) - 4} lines of data) into {OUTPUT_FILENAME}...")
    
    try:
        with open(OUTPUT_FILENAME, 'w') as f:
            f.write(mxmod_data)
        print(f"Successfully created {OUTPUT_FILENAME}.")
        print("This file contains the inverted 3D cross, ready to be used as an inner skybox.")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    main()