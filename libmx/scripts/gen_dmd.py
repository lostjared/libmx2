import sys

def generate_diamond_mxmod(filename="diamond.mxmod", size=5.0):
    """
    Generates a hollow double-pyramid (octahedron) for a skybox.
    Format: .mxmod (vert block, tex block, norm block).
    Winding: Inward facing (for camera inside).
    """

    # --- 1. Define Key Points ---
    top    = (0.0,  size, 0.0)
    bottom = (0.0, -size, 0.0)
    
    front  = (0.0, 0.0,  size)
    right  = ( size, 0.0, 0.0)
    back   = (0.0, 0.0, -size)
    left   = (-size, 0.0, 0.0)

    # --- 2. Define UV Coordinates ---
    # Simple mapping: Top/Bottom at V=1/0, Equator at V=0.5
    uv_top    = (0.5, 1.0)
    uv_bottom = (0.5, 0.0)
    
    uv_front  = (0.0, 0.5)
    uv_right  = (0.25, 0.5)
    uv_back   = (0.5, 0.5)
    uv_left   = (0.75, 0.5)
    uv_front_wrap = (1.0, 0.5)

    # --- 3. Define Faces ---
    # Each face is a tuple of 3 vertices and 3 UVs: (v1, v2, v3, uv1, uv2, uv3)
    # Winding order is chosen to generate INWARD pointing normals.
    faces = []

    # -- Top Pyramid --
    # 1. Front-Right Face (Top -> Right -> Front)
    faces.append((top, right, front, uv_top, uv_right, uv_front))
    # 2. Right-Back Face (Top -> Back -> Right)
    faces.append((top, back, right, uv_top, uv_back, uv_right))
    # 3. Back-Left Face (Top -> Left -> Back)
    faces.append((top, left, back, uv_top, uv_left, uv_back))
    # 4. Left-Front Face (Top -> Front -> Left)
    faces.append((top, front, left, uv_top, uv_front_wrap, uv_left))

    # -- Bottom Pyramid --
    # 5. Front-Right Face (Bottom -> Front -> Right)
    faces.append((bottom, front, right, uv_bottom, uv_front, uv_right))
    # 6. Right-Back Face (Bottom -> Right -> Back)
    faces.append((bottom, right, back, uv_bottom, uv_right, uv_back))
    # 7. Back-Left Face (Bottom -> Back -> Left)
    faces.append((bottom, back, left, uv_bottom, uv_back, uv_left))
    # 8. Left-Front Face (Bottom -> Left -> Front)
    faces.append((bottom, left, front, uv_bottom, uv_left, uv_front_wrap))

    # --- 4. Process Data ---
    out_verts = []
    out_texs = []
    out_norms = []

    for v1, v2, v3, t1, t2, t3 in faces:
        # Append Vertices
        out_verts.extend([v1, v2, v3])
        # Append UVs
        out_texs.extend([t1, t2, t3])
        
        # Calculate Face Normal (Cross Product)
        # Vector U = v2 - v1
        ux, uy, uz = v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]
        # Vector V = v3 - v1
        vx, vy, vz = v3[0]-v1[0], v3[1]-v1[1], v3[2]-v1[2]
        
        # Cross Product (Nx, Ny, Nz)
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        
        # Normalize
        length = (nx*nx + ny*ny + nz*nz)**0.5
        if length == 0: length = 1.0
        norm = (nx/length, ny/length, nz/length)
        
        # Append Normal (duplicated for each vertex of the face for flat shading)
        out_norms.extend([norm, norm, norm])

    num_vertices = len(out_verts)

    # --- 5. Write File ---
    try:
        with open(filename, 'w') as f:
            # Header
            f.write("tri 0 0\n")
            
            # Vertices Block
            f.write(f"vert {num_vertices}\n")
            for v in out_verts:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n") # Spacer

            # Texture Coordinates Block
            f.write(f"tex {num_vertices}\n")
            for t in out_texs:
                f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            f.write("\n") # Spacer

            # Normals Block
            f.write(f"norm {num_vertices}\n")
            for n in out_norms:
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        
        print(f"Successfully generated {filename} with {num_vertices} vertices.")

    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    generate_diamond_mxmod("diamond.mxmod", size=5)
