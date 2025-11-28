import math

def create_skybox_hexagon(filename):
    # Configuration for a large skybox
    # Large radius and height to ensure plenty of room for the camera
    radius = 1000.0
    height = 1000.0
    y_top = height / 2.0
    y_bottom = -height / 2.0
    
    positions = []
    tex_coords = []
    normals = []
    
    def add_vertex(x, y, z, u, v, nx, ny, nz):
        positions.append((x, y, z))
        tex_coords.append((u, v))
        normals.append((nx, ny, nz))

    # --- Top Face (Ceiling) ---
    # Normal points DOWN (0, -1, 0) because we are inside looking up
    for i in range(6):
        theta1 = math.radians(i * 60)
        theta2 = math.radians((i + 1) * 60)
        
        cx, cy, cz = 0.0, y_top, 0.0
        
        x1 = math.cos(theta1) * radius
        z1 = math.sin(theta1) * radius
        x2 = math.cos(theta2) * radius
        z2 = math.sin(theta2) * radius
        
        nx, ny, nz = 0.0, -1.0, 0.0
        
        # UVs (Planar mapping)
        cu, cv = 0.5, 0.5
        u1 = (x1 / radius) * 0.5 + 0.5
        v1 = (z1 / radius) * 0.5 + 0.5
        u2 = (x2 / radius) * 0.5 + 0.5
        v2 = (z2 / radius) * 0.5 + 0.5
        
        # Winding: To face inward (down), we need Clockwise looking from top
        # (which is CCW looking from inside).
        # Order: Center -> P2 -> P1
        add_vertex(cx, cy, cz, cu, cv, nx, ny, nz)
        add_vertex(x2, y_top, z2, u2, v2, nx, ny, nz)
        add_vertex(x1, y_top, z1, u1, v1, nx, ny, nz)

    # --- Bottom Face (Floor) ---
    # Normal points UP (0, 1, 0) because we are inside looking down
    for i in range(6):
        theta1 = math.radians(i * 60)
        theta2 = math.radians((i + 1) * 60)
        
        cx, cy, cz = 0.0, y_bottom, 0.0
        
        x1 = math.cos(theta1) * radius
        z1 = math.sin(theta1) * radius
        x2 = math.cos(theta2) * radius
        z2 = math.sin(theta2) * radius
        
        nx, ny, nz = 0.0, 1.0, 0.0
        
        cu, cv = 0.5, 0.5
        u1 = (x1 / radius) * 0.5 + 0.5
        v1 = (z1 / radius) * 0.5 + 0.5
        u2 = (x2 / radius) * 0.5 + 0.5
        v2 = (z2 / radius) * 0.5 + 0.5
        
        # Winding: To face inward (up), we need CCW looking from top.
        # Order: Center -> P1 -> P2
        add_vertex(cx, cy, cz, cu, cv, nx, ny, nz)
        add_vertex(x1, y_bottom, z1, u1, v1, nx, ny, nz)
        add_vertex(x2, y_bottom, z2, u2, v2, nx, ny, nz)

    # --- Side Faces (Walls) ---
    # Normals point INWARD towards (0,0,0)
    for i in range(6):
        theta1 = math.radians(i * 60)
        theta2 = math.radians((i + 1) * 60)
        
        x1 = math.cos(theta1) * radius
        z1 = math.sin(theta1) * radius
        x2 = math.cos(theta2) * radius
        z2 = math.sin(theta2) * radius
        
        # Normal calculation (Inward)
        mid_x = (x1 + x2) / 2.0
        mid_z = (z1 + z2) / 2.0
        length = math.sqrt(mid_x*mid_x + mid_z*mid_z)
        nx = -mid_x / length
        ny = 0.0
        nz = -mid_z / length
        
        # UVs (0-1 per face)
        u_left = 0.0
        u_right = 1.0
        v_top = 0.0
        v_bottom = 1.0
        
        # Vertices for the quad
        # TL: (x1, y_top, z1)
        # TR: (x2, y_top, z2)
        # BL: (x1, y_bottom, z1)
        # BR: (x2, y_bottom, z2)
        
        # Winding for inward facing (CCW from inside perspective):
        # Triangle 1: Top-Left -> Top-Right -> Bottom-Left
        add_vertex(x1, y_top, z1, u_left, v_top, nx, ny, nz)       # TL
        add_vertex(x2, y_top, z2, u_right, v_top, nx, ny, nz)      # TR
        add_vertex(x1, y_bottom, z1, u_left, v_bottom, nx, ny, nz) # BL
        
        # Triangle 2: Top-Right -> Bottom-Right -> Bottom-Left
        add_vertex(x2, y_top, z2, u_right, v_top, nx, ny, nz)      # TR
        add_vertex(x2, y_bottom, z2, u_right, v_bottom, nx, ny, nz)# BR
        add_vertex(x1, y_bottom, z1, u_left, v_bottom, nx, ny, nz) # BL

    # Write to MXMOD format
    count = len(positions)
    with open(filename, 'w') as f:
        f.write("tri 0 0\n")
        f.write(f"vert {count}\n")
        for p in positions:
            f.write(f"{p[0]:.6f} {p[1]:.6f} {p[2]:.6f}\n")
        f.write("\n")
        
        f.write(f"tex {count}\n")
        for t in tex_coords:
            f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
        f.write("\n")
        
        f.write(f"norm {count}\n")
        for n in normals:
            f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
            
    
if __name__ == "__main__":
    create_skybox_hexagon("skybox_hex.mxmod")