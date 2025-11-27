import math

def create_tunnel_skybox(filename):
    # Configuration - Massive dimensions for Skybox usage
    radius = 50.0           
    cylinder_length = 200.0 
    pyramid_height = 50.0   
    segments = 32           # Higher segment count for smoother roundness
    texture_repeat_z = 5.0  # Repeat texture along length to prevent stretching
    
    # Lists to hold the raw data
    positions = []
    tex_coords = []
    normals = []

    half_len = cylinder_length / 2.0
    
    # Helper to add a single vertex
    def add_vertex(x, y, z, u, v, nx, ny, nz):
        positions.append((x, y, z))
        tex_coords.append((u, v))
        normals.append((nx, ny, nz))

    # --- 1. Cylinder Body (The Tunnel) ---
    for i in range(segments):
        # Angles
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        # Coordinates on circle
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        # Normals (POINTING INWARD for inside view)
        # We negate the cosine/sine
        n1x, n1y = -math.cos(theta1), -math.sin(theta1)
        n2x, n2y = -math.cos(theta2), -math.sin(theta2)
        
        # UVs
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Z positions
        z_back = -half_len
        z_front = half_len
        
        # WINDING ORDER FLIPPED:
        # To see the face from the inside, we must wind the opposite direction 
        # compared to the exterior ship model.
        
        # Triangle 1: Back-Left -> Front-Left -> Back-Right
        add_vertex(x1, y1, z_back, u1, 0.0, n1x, n1y, 0.0)
        add_vertex(x1, y1, z_front, u1, texture_repeat_z, n1x, n1y, 0.0)
        add_vertex(x2, y2, z_back, u2, 0.0, n2x, n2y, 0.0)
        
        # Triangle 2: Back-Right -> Front-Left -> Front-Right
        add_vertex(x2, y2, z_back, u2, 0.0, n2x, n2y, 0.0)
        add_vertex(x1, y1, z_front, u1, texture_repeat_z, n1x, n1y, 0.0)
        add_vertex(x2, y2, z_front, u2, texture_repeat_z, n2x, n2y, 0.0)

    # --- 2. Front Pyramid (End of Tunnel at +Z) ---
    # Tip is further out
    tip_z = half_len + pyramid_height
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Calculate INWARD facing normal
        # V1=(x1,y1,half), V2=(x2,y2,half), Tip=(0,0,tip)
        # Normal points towards the axis
        ax, ay, az = x2-x1, y2-y1, 0
        bx, by, bz = -x1, -y1, tip_z - half_len
        
        # Cross product
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        
        # Flip normal for inside view
        nx, ny, nz = -nx, -ny, -nz
        
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        nx, ny, nz = nx/length, ny/length, nz/length

        # Winding Order Flipped for Inside View
        # Base 2 -> Base 1 -> Tip
        add_vertex(x2, y2, half_len, u2, 0.0, nx, ny, nz)
        add_vertex(x1, y1, half_len, u1, 0.0, nx, ny, nz)
        add_vertex(0.0, 0.0, tip_z, (u1+u2)/2, 1.0, nx, ny, nz)

    # --- 3. Back Pyramid (End of Tunnel at -Z) ---
    tip_z = -half_len - pyramid_height
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Calculate INWARD facing normal
        ax, ay, az = x1-x2, y1-y2, 0
        bx, by, bz = -x2, -y2, tip_z - (-half_len)
        
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        
        # Flip normal for inside view
        nx, ny, nz = -nx, -ny, -nz
        
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        nx, ny, nz = nx/length, ny/length, nz/length

        # Winding Order Flipped for Inside View
        # Base 1 -> Base 2 -> Tip
        add_vertex(x1, y1, -half_len, u1, 0.0, nx, ny, nz)
        add_vertex(x2, y2, -half_len, u2, 0.0, nx, ny, nz)
        add_vertex(0.0, 0.0, tip_z, (u1+u2)/2, 1.0, nx, ny, nz)

    # --- Write File ---
    count = len(positions)
    
    with open(filename, 'w') as f:
        # Header
        f.write("tri 0 0\n")
        
        # Vertices
        f.write(f"vert {count}\n")
        for p in positions:
            f.write(f"{p[0]:.6f} {p[1]:.6f} {p[2]:.6f}\n")
        
        f.write("\n")
            
        # Texture Coords
        f.write(f"tex {count}\n")
        for t in tex_coords:
            f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            
        f.write("\n")

        # Normals
        f.write(f"norm {count}\n")
        for n in normals:
            f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")

    print(f"Generated {filename} with {count} vertices (Inside-facing).")

if __name__ == "__main__":
    create_tunnel_skybox("tunnel_skybox.mxmod")