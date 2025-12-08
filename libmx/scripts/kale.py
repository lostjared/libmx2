import math

def generate_kaleidoscope_mxmod(filename="kaleidoscope.mxmod"):
    # --- CONFIGURATION ---
    SIDES = 6           # 3 = Triangle (Classic), 6 = Hexagon (Tunnel), 8 = Cylinder-ish
    RADIUS = 50.0       # How wide the tunnel is
    LENGTH = 200.0      # How long the tunnel is
    
    # Vertices calculation
    # We need a "Near" ring and a "Far" ring
    z_near = -LENGTH / 2.0
    z_far =  LENGTH / 2.0
    
    out_verts = []
    out_uvs = []
    out_norms = []

    angle_step = (2 * math.pi) / SIDES

    for i in range(SIDES):
        # Calculate angles for the current panel
        theta_current = i * angle_step
        theta_next = (i + 1) * angle_step

        # --- 1. Calculate Positions (X, Y) ---
        # Current Corner
        x1 = RADIUS * math.cos(theta_current)
        y1 = RADIUS * math.sin(theta_current)
        
        # Next Corner
        x2 = RADIUS * math.cos(theta_next)
        y2 = RADIUS * math.sin(theta_next)

        # Define the 4 corners of this wall segment
        # We wind them Counter-Clockwise from the INSIDE perspective
        v_bl = (x1, y1, z_near) # Bottom Left
        v_br = (x2, y2, z_near) # Bottom Right
        v_tr = (x2, y2, z_far)  # Top Right
        v_tl = (x1, y1, z_far)  # Top Left

        # --- 2. Calculate Normal (Pointing Inward) ---
        # The normal is simply the inverse of the direction from center to the wall
        mid_theta = (theta_current + theta_next) / 2.0
        nx = -math.cos(mid_theta) # Negative to point IN
        ny = -math.sin(mid_theta) # Negative to point IN
        nz = 0.0
        normal = (nx, ny, nz)

        # --- 3. Calculate UVs with Mirroring ---
        # To make a kaleidoscope, we flip the texture horizontally on every odd face
        if i % 2 == 0:
            # Standard UVs
            uv_bl = (0.0, 0.0)
            uv_br = (1.0, 0.0)
            uv_tr = (1.0, 1.0)
            uv_tl = (0.0, 1.0)
        else:
            # Mirrored UVs (Flip X)
            uv_bl = (1.0, 0.0)
            uv_br = (0.0, 0.0)
            uv_tr = (0.0, 1.0)
            uv_tl = (1.0, 1.0)

        # --- 4. Build Triangles ---
        # Triangle 1: BL -> BR -> TR
        out_verts.extend([v_bl, v_br, v_tr])
        out_uvs.extend([uv_bl, uv_br, uv_tr])
        out_norms.extend([normal, normal, normal])

        # Triangle 2: BL -> TR -> TL
        out_verts.extend([v_bl, v_tr, v_tl])
        out_uvs.extend([uv_bl, uv_tr, uv_tl])
        out_norms.extend([normal, normal, normal])

    # --- WRITE FILE ---
    vertex_count = len(out_verts)
    
    with open(filename, "w") as f:
        f.write("tri 0 0\n")
        
        # Write Vertices
        f.write(f"vert {vertex_count}\n")
        for v in out_verts:
            f.write(f"{v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
        f.write("\n")
        
        # Write Textures
        f.write(f"tex {vertex_count}\n")
        for uv in out_uvs:
            f.write(f"{uv[0]:.4f} {uv[1]:.4f}\n")
        f.write("\n")
        
        # Write Normals
        f.write(f"norm {vertex_count}\n")
        for n in out_norms:
            f.write(f"{n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")

    print(f"File saved: {filename}")
    print(f"Shape: {SIDES}-sided Prism")
    print(f"Texture Mirroring: Enabled")

if __name__ == "__main__":
    generate_kaleidoscope_mxmod()