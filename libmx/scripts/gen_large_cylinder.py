import math

def create_cigar_ship(filename):
    # Configuration - Scaled 10x
    radius = 5.0            # Was 0.5
    cylinder_length = 40.0  # Was 4.0
    pyramid_height = 15.0   # Was 1.5
    segments = 16  # Smoothness
    
    # Lists to hold the raw data
    positions = []
    tex_coords = []
    normals = []

    half_len = cylinder_length / 2.0
    
    # Helper to add a single vertex to the lists
    def add_vertex(x, y, z, u, v, nx, ny, nz):
        positions.append((x, y, z))
        tex_coords.append((u, v))
        normals.append((nx, ny, nz))

    # --- 1. Cylinder Body ---
    for i in range(segments):
        # Angles
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        # Coordinates on circle
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        # Normals (pointing out)
        n1x, n1y = math.cos(theta1), math.sin(theta1)
        n2x, n2y = math.cos(theta2), math.sin(theta2)
        
        # UVs
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Z positions
        z_back = -half_len
        z_front = half_len
        
        # Triangle 1 (Back-Bottom-Left -> Back-Top-Right -> Front-Bottom-Left)
        # Note: Winding order is usually CCW
        
        # Quad Vertex 1 (Back, Angle 1)
        add_vertex(x1, y1, z_back, u1, 0.0, n1x, n1y, 0.0)
        # Quad Vertex 2 (Back, Angle 2)
        add_vertex(x2, y2, z_back, u2, 0.0, n2x, n2y, 0.0)
        # Quad Vertex 3 (Front, Angle 1)
        add_vertex(x1, y1, z_front, u1, 1.0, n1x, n1y, 0.0)
        
        # Triangle 2
        # Quad Vertex 2 (Back, Angle 2)
        add_vertex(x2, y2, z_back, u2, 0.0, n2x, n2y, 0.0)
        # Quad Vertex 4 (Front, Angle 2)
        add_vertex(x2, y2, z_front, u2, 1.0, n2x, n2y, 0.0)
        # Quad Vertex 3 (Front, Angle 1)
        add_vertex(x1, y1, z_front, u1, 1.0, n1x, n1y, 0.0)

    # --- 2. Front Pyramid (Tip at +Z) ---
    tip_z = half_len + pyramid_height
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Calculate face normal for pyramid
        # V1 = (x1, y1, half_len), V2 = (x2, y2, half_len), Tip = (0,0,tip_z)
        # Vector A = V2 - V1
        # Vector B = Tip - V1
        ax, ay, az = x2-x1, y2-y1, 0
        bx, by, bz = -x1, -y1, tip_z - half_len
        
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        nx, ny, nz = nx/length, ny/length, nz/length

        # Base 1
        add_vertex(x1, y1, half_len, u1, 0.0, nx, ny, nz)
        # Base 2
        add_vertex(x2, y2, half_len, u2, 0.0, nx, ny, nz)
        # Tip
        add_vertex(0.0, 0.0, tip_z, (u1+u2)/2, 1.0, nx, ny, nz)

    # --- 3. Back Pyramid (Tip at -Z) ---
    tip_z = -half_len - pyramid_height
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1, y1 = math.cos(theta1) * radius, math.sin(theta1) * radius
        x2, y2 = math.cos(theta2) * radius, math.sin(theta2) * radius
        
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Calculate face normal
        # V1 = (x2, y2, -half_len), V2 = (x1, y1, -half_len), Tip = (0,0,tip_z)
        ax, ay, az = x1-x2, y1-y2, 0
        bx, by, bz = -x2, -y2, tip_z - (-half_len)
        
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        nx, ny, nz = nx/length, ny/length, nz/length

        # Base 2 (Order swapped for winding)
        add_vertex(x2, y2, -half_len, u2, 0.0, nx, ny, nz)
        # Base 1
        add_vertex(x1, y1, -half_len, u1, 0.0, nx, ny, nz)
        # Tip
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

        print("genreated: " + filename)

if  __name__ == "__main__":
    create_cigar_ship("large_cylinder.mxmod")