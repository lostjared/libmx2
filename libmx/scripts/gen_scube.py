import numpy as np

def generate_mxmod_pyramid_cube(pyramid_height=0.4, output_filename="pyramid_cube_skybox.mxmod"):
    """
    Generates a cube with pyramids on each face (stellated cube) in the MXMOD format.
    The data is generated for 48 triangles (144 vertices).
    """

    # --- 1. Define Base and Apex Vertices ---
    
    # Cube half-side length
    s = 0.5
    
    # 8 Base Cube Vertices (The corners)
    v_base = np.array([
        [-s, -s, -s], [ s, -s, -s], [ s,  s, -s], [-s,  s, -s], # Z-
        [-s, -s,  s], [ s, -s,  s], [ s,  s,  s], [-s,  s,  s]  # Z+
    ])

    # 6 Pyramid Apex Vertices (The points)
    h = s + pyramid_height
    v_apex = np.array([
        [ h,  0,  0],  # 8: +X
        [-h,  0,  0],  # 9: -X
        [ 0,  h,  0],  # 10: +Y
        [ 0, -h,  0],  # 11: -Y
        [ 0,  0,  h],  # 12: +Z
        [ 0,  0, -h]   # 13: -Z
    ])

    # Total vertices for geometry calculation (0-indexed: 0 to 13)
    v_all = np.vstack([v_base, v_apex])
    
    # --- 2. Define Pyramid Faces (1-indexed for reference) ---

    # Define the 6 pyramids by their 4 base corners and 1 apex
    # (Corner indices are 0-7, Apex indices are 8-13)
    # The order of base vertices must be consistent (e.g., counter-clockwise) 
    # to maintain outward-facing normals.
    
    # Structure: (V1, V2, V3, V4 are cube corners, A is the apex)
    # Face 1 (V1, V2, A), Face 2 (V2, V3, A), Face 3 (V3, V4, A), Face 4 (V4, V1, A)
    pyramid_definitions = [
        # +Z (Top): Corners 4, 5, 6, 7. Apex 12.
        ([4, 5, 6, 7], 12),
        # -Z (Bottom): Corners 3, 2, 1, 0. Apex 13. (Reverse order for outward normal)
        ([3, 2, 1, 0], 13),
        # +Y (Front): Corners 7, 6, 2, 3. Apex 10.
        ([7, 6, 2, 3], 10),
        # -Y (Back): Corners 4, 0, 1, 5. Apex 11.
        ([4, 0, 1, 5], 11),
        # +X (Right): Corners 5, 1, 2, 6. Apex 8.
        ([5, 1, 2, 6], 8),
        # -X (Left): Corners 7, 3, 0, 4. Apex 9.
        ([7, 3, 0, 4], 9),
    ]

    all_verts = []
    all_uvs = []
    all_norms = []
    
    # Standard UV coordinates for a single quad (0,0 to 1,1)
    # These will be reused for every triangle face
    uv_tri = np.array([
        [0.0, 0.0],  # Corner 1 of quad
        [1.0, 0.0],  # Corner 2 of quad
        [0.5, 1.0]   # Apex point of quad (reprojected to 0.5, 1.0)
    ])

    # --- 3. Generate Vertex Data for 48 Triangles (144 Vertices) ---
    
    # We generate a list of 4 triangles for each of the 6 pyramid faces.
    for corners, apex_idx in pyramid_definitions:
        V1, V2, V3, V4 = corners
        A = apex_idx

        # Define the 4 triangles for this pyramid face
        # We use 0-indexed values here
        triangles = [
            (V1, V2, A),
            (V2, V3, A),
            (V3, V4, A),
            (V4, V1, A)
        ]
        
        # Calculate a single normal vector for the pyramid's face 
        # (Average of the base vertices is a good approximation for the center)
        base_center = np.mean(v_all[corners], axis=0)
        apex_pos = v_all[A]
        # The normal points from the base center toward the apex (outward)
        face_normal = apex_pos - base_center
        face_normal = face_normal / np.linalg.norm(face_normal)
        
        # Append data for each of the 4 triangles
        for i, (v_a, v_b, v_c) in enumerate(triangles):
            # 1. Vertices (positions)
            all_verts.extend([v_all[v_a], v_all[v_b], v_all[v_c]])
            
            # 2. UVs (Texture coordinates)
            # Simple triangular UVs that map the texture from the base edge to the apex
            all_uvs.append([0.0, 0.0]) # V1/V2/V3/V4 corner 
            all_uvs.append([1.0, 0.0]) # V2/V3/V4/V1 corner
            all_uvs.append([0.5, 1.0]) # Apex corner

            # 3. Normals
            # Use the calculated face normal for all three vertices of the triangle
            all_norms.extend([face_normal, face_normal, face_normal])

    # --- 4. Final Data Compilation and MXMOD File Generation ---

    final_verts = np.array(all_verts)
    final_uvs = np.array(all_uvs)
    final_norms = np.array(all_norms)
    
    total_verts = len(final_verts)
    # Total triangles is total_verts / 3
    
    # Flip Normals INWARD for a Skybox View
    final_norms *= -1
    
    with open(output_filename, 'w') as f:
        # Write the header lines
        f.write(f"tri {total_verts // 3} 0\n") 
        f.write(f"vert {total_verts}\n")
        
        # Write Vertices
        for v in final_verts:
            f.write(f"{v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")

        # Write UVs
        f.write(f"\ntex {total_verts}\n")
        for uv in final_uvs:
            f.write(f"{uv[0]:.4f} {uv[1]:.4f}\n")

        # Write Normals
        f.write(f"\nnorm {total_verts}\n")
        for n in final_norms:
            f.write(f"{n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")

    print(f"✅ Successfully generated MXMOD model for the Pyramid Cube: {output_filename} ({total_verts} vertices)")

# Execute the script
# Adjust pyramid_height to control the stretch: higher value = more acute (pointy) pyramid.
generate_mxmod_pyramid_cube(pyramid_height=0.4)