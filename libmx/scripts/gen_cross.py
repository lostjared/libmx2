import sys

def generate_cross_mxmod(filename="cross_skybox.mxmod", arm_length=20.0, arm_width=5.0):
    """
    Generates a hollow 3D Cross (6 intersecting square prisms) for a skybox.
    - Camera at (0,0,0).
    - Normals point INWARD.
    - 6 Arms: +X, -X, +Y, -Y, +Z, -Z.
    """
    
    vertices = []
    tex_coords = []
    normals = []
    
    hw = arm_width / 2.0
    
    # --- Helper: Add Triangle with Auto-Inward Normal ---
    def add_triangle(p1, p2, p3, t1, t2, t3):
        # 1. Calc Normal
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        
        # Normalize
        l = (nx*nx + ny*ny + nz*nz)**0.5
        if l > 0: nx, ny, nz = nx/l, ny/l, nz/l
        
        # 2. Ensure Inward (Dot product with position should be negative for inward facing? 
        # Actually, if camera is at 0,0,0, vector P is outward. 
        # Inward normal should oppose P. So Dot(N, P) < 0.
        # If Dot > 0, it's outward, so flip.)
        
        # Use centroid for check
        cx = (p1[0]+p2[0]+p3[0])/3.0
        cy = (p1[1]+p2[1]+p3[1])/3.0
        cz = (p1[2]+p2[2]+p3[2])/3.0
        
        if (nx*cx + ny*cy + nz*cz) > 0:
            nx, ny, nz = -nx, -ny, -nz
            p2, p3 = p3, p2 # Flip winding
            t2, t3 = t3, t2

        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([(nx, ny, nz)] * 3)

    def add_quad(p1, p2, p3, p4):
        # UVs 0-1
        t1, t2, t3, t4 = (0,0), (1,0), (1,1), (0,1)
        add_triangle(p1, p2, p3, t1, t2, t3)
        add_triangle(p1, p3, p4, t1, t3, t4)

    # --- Generate Arms ---
    # We generate the 4 walls and 1 cap for each of the 6 arms.
    # The "start" of the arm is at offset 'hw' from center (the central cube face).
    # The "end" is at 'arm_length'.
    
    # Define the 6 directions and their axes
    # Axis: 0=X, 1=Y, 2=Z
    # Dir: 1 or -1
    arms = [
        (0,  1), # +X
        (0, -1), # -X
        (1,  1), # +Y
        (1, -1), # -Y
        (2,  1), # +Z
        (2, -1)  # -Z
    ]

    for axis, direction in arms:
        # Determine coordinates based on axis
        # We construct the arm in a local frame (Forward, Right, Up) and map to global
        
        start = hw * direction
        end = arm_length * direction
        
        # The 4 corners of the profile (Right/Up plane relative to arm axis)
        # We iterate 4 walls.
        # Let's define the 4 corners of the square cross-section at 'start' and 'end'
        
        # Helper to map local (depth, u, v) to global (x, y, z)
        def map_coords(d, u, v):
            if axis == 0: return (d, u, v) # X is depth
            if axis == 1: return (v, d, u) # Y is depth
            return (u, v, d)               # Z is depth

        # Corners in (u, v) local space (spanning -hw to hw)
        corners = [
            ( hw,  hw), # Top-Right
            ( hw, -hw), # Bottom-Right
            (-hw, -hw), # Bottom-Left
            (-hw,  hw)  # Top-Left
        ]
        
        # 1. Generate 4 Walls
        for i in range(4):
            u1, v1 = corners[i]
            u2, v2 = corners[(i+1)%4]
            
            # Wall Quad:
            # Near1 -> Near2 -> Far2 -> Far1 (or similar winding)
            # We rely on add_triangle to fix winding/normals.
            
            p1 = map_coords(start, u1, v1)
            p2 = map_coords(start, u2, v2)
            p3 = map_coords(end, u2, v2)
            p4 = map_coords(end, u1, v1)
            
            add_quad(p1, p2, p3, p4)

        # 2. Generate End Cap
        # Quad at depth 'end'
        c1 = map_coords(end, corners[0][0], corners[0][1])
        c2 = map_coords(end, corners[1][0], corners[1][1])
        c3 = map_coords(end, corners[2][0], corners[2][1])
        c4 = map_coords(end, corners[3][0], corners[3][1])
        
        add_quad(c1, c2, c3, c4)

    # --- Write File ---
    num_verts = len(vertices)
    try:
        with open(filename, 'w') as f:
            f.write("tri 0 0\n")
            f.write(f"vert {num_verts}\n")
            for v in vertices:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n")
            f.write(f"tex {num_verts}\n")
            for t in tex_coords:
                f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            f.write("\n")
            f.write(f"norm {num_verts}\n")
            for n in normals:
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        print(f"Successfully generated {filename} with {num_verts} vertices.")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    generate_cross_mxmod()