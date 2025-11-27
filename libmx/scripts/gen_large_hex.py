import math

def create_hexagon(filename):
    # Configuration
    radius = 50.0
    y_pos = 0.0
    
    positions = []
    tex_coords = []
    normals = []
    
    def add_vertex(x, y, z, u, v, nx, ny, nz):
        positions.append((x, y, z))
        tex_coords.append((u, v))
        normals.append((nx, ny, nz))

    # Generate 6 triangles for the hexagon fan
    for i in range(6):
        # Angles in radians (60 degrees per segment)
        theta1 = math.radians(i * 60)
        theta2 = math.radians((i + 1) * 60)
        
        # Center point
        cx, cy, cz = 0.0, y_pos, 0.0
        
        # Outer points
        x1 = math.cos(theta1) * radius
        z1 = math.sin(theta1) * radius
        
        x2 = math.cos(theta2) * radius
        z2 = math.sin(theta2) * radius
        
        # Normal (Pointing Up)
        nx, ny, nz = 0.0, 1.0, 0.0
        
        # UV Coordinates (Planar mapping)
        # Map range [-radius, radius] to [0, 1]
        cu, cv = 0.5, 0.5
        u1 = (x1 / radius) * 0.5 + 0.5
        v1 = (z1 / radius) * 0.5 + 0.5
        u2 = (x2 / radius) * 0.5 + 0.5
        v2 = (z2 / radius) * 0.5 + 0.5
        
        # Triangle (Center -> P1 -> P2 is CCW for Up normal)
        add_vertex(cx, cy, cz, cu, cv, nx, ny, nz)
        add_vertex(x1, y_pos, z1, u1, v1, nx, ny, nz)
        add_vertex(x2, y_pos, z2, u2, v2, nx, ny, nz)

    # Write to file
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
            
    print(f"Generated {filename} with {count} vertices.")

if __name__ == "__main__":
    create_hexagon("hexagon.mxmod")