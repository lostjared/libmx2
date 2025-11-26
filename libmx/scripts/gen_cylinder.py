import math

def write_mxmod(filename):
    vertices = []
    
    # Configuration
    tube_radius = 1.0
    tube_length = 8.0
    segments = 16  # Number of sides for cylinder/pyramids
    pyramid_height = 2.5
    
    half_length = tube_length / 2.0
    
    # --- Helper to add a triangle ---
    def add_tri(p1, t1, n1, p2, t2, n2, p3, t3, n3):
        # Format: v x y z t u v n nx ny nz
        vertices.append(f"v {p1[0]:.4f} {p1[1]:.4f} {p1[2]:.4f} t {t1[0]:.4f} {t1[1]:.4f} n {n1[0]:.4f} {n1[1]:.4f} {n1[2]:.4f}")
        vertices.append(f"v {p2[0]:.4f} {p2[1]:.4f} {p2[2]:.4f} t {t2[0]:.4f} {t2[1]:.4f} n {n2[0]:.4f} {n2[1]:.4f} {n2[2]:.4f}")
        vertices.append(f"v {p3[0]:.4f} {p3[1]:.4f} {p3[2]:.4f} t {t3[0]:.4f} {t3[1]:.4f} n {n3[0]:.4f} {n3[1]:.4f} {n3[2]:.4f}")

    # --- 1. Generate Cylinder Body ---
    for i in range(segments):
        # Calculate angles
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        # Calculate x, y coordinates (tube runs along Z axis for length)
        # Actually, let's make it run along Z to match typical "forward" direction
        x1 = math.cos(theta1) * tube_radius
        y1 = math.sin(theta1) * tube_radius
        x2 = math.cos(theta2) * tube_radius
        y2 = math.sin(theta2) * tube_radius
        
        # Texture coords
        u1 = i / segments
        u2 = (i + 1) / segments
        
        # Normals (pointing out from center)
        n1 = (math.cos(theta1), math.sin(theta1), 0)
        n2 = (math.cos(theta2), math.sin(theta2), 0)
        
        # Positions
        # Back circle (z = -half_length)
        p1_back = (x1, y1, -half_length)
        p2_back = (x2, y2, -half_length)
        
        # Front circle (z = half_length)
        p1_front = (x1, y1, half_length)
        p2_front = (x2, y2, half_length)
        
        # Triangle 1 (Bottom-Left to Top-Right)
        add_tri(p1_back, (u1, 0), n1, 
                p2_back, (u2, 0), n2, 
                p1_front, (u1, 1), n1)
                
        # Triangle 2 (Top-Right to Bottom-Left)
        add_tri(p2_back, (u2, 0), n2, 
                p2_front, (u2, 1), n2, 
                p1_front, (u1, 1), n1)

    # --- 2. Generate Front Pyramid (Tip at +Z) ---
    tip_front = (0, 0, half_length + pyramid_height)
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1 = math.cos(theta1) * tube_radius
        y1 = math.sin(theta1) * tube_radius
        x2 = math.cos(theta2) * tube_radius
        y2 = math.sin(theta2) * tube_radius
        
        # Base points
        p1 = (x1, y1, half_length)
        p2 = (x2, y2, half_length)
        
        # Calculate face normal
        # Vector A = p2 - p1
        # Vector B = tip - p1
        # Normal = Cross(A, B)
        ax, ay, az = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        bx, by, bz = tip_front[0]-p1[0], tip_front[1]-p1[1], tip_front[2]-p1[2]
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        normal = (nx/length, ny/length, nz/length)
        
        u1 = i / segments
        u2 = (i + 1) / segments
        
        add_tri(p1, (u1, 0), normal,
                p2, (u2, 0), normal,
                tip_front, ((u1+u2)/2, 1), normal)

    # --- 3. Generate Back Pyramid (Tip at -Z) ---
    tip_back = (0, 0, -half_length - pyramid_height)
    
    for i in range(segments):
        theta1 = (i / segments) * 2 * math.pi
        theta2 = ((i + 1) / segments) * 2 * math.pi
        
        x1 = math.cos(theta1) * tube_radius
        y1 = math.sin(theta1) * tube_radius
        x2 = math.cos(theta2) * tube_radius
        y2 = math.sin(theta2) * tube_radius
        
        # Base points (Note order for winding)
        p1 = (x2, y2, -half_length)
        p2 = (x1, y1, -half_length)
        
        # Calculate face normal
        ax, ay, az = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        bx, by, bz = tip_back[0]-p1[0], tip_back[1]-p1[1], tip_back[2]-p1[2]
        nx = ay*bz - az*by
        ny = az*bx - ax*bz
        nz = ax*by - ay*bx
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        normal = (nx/length, ny/length, nz/length)
        
        u1 = (i + 1) / segments
        u2 = i / segments
        
        add_tri(p1, (u1, 0), normal,
                p2, (u2, 0), normal,
                tip_back, ((u1+u2)/2, 1), normal)

    # Write to file
    with open(filename, 'w') as f:
        # Header (optional based on your loader, but standard for mxmod)
        f.write(f"{len(vertices)}\n") 
        for v in vertices:
            f.write(v + "\n")
            
    print(f"Successfully created {filename} with {len(vertices)} vertices.")

if __name__ == "__main__":
    write_mxmod("tube_ship.mxmod")