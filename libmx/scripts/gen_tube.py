import math
import sys

def generate_cigar_mxmod(filename="cigar_tunnel.mxmod", 
                         radius=5.0, 
                         tunnel_length=20.0, 
                         cap_length=5.0, 
                         segments=16):
    """
    Generates a cigar-shaped tunnel (cylinder with cone caps) .mxmod file.
    - Camera is assumed to be inside.
    - Normals point INWARD.
    - Axis is along Z.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # --- Helper to add a triangle ---
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([n1, n2, n3])

    # --- 1. Generate Central Cylinder (Tunnel) ---
    # Z ranges from -tunnel_length/2 to +tunnel_length/2
    z_start = -tunnel_length / 2.0
    z_end   =  tunnel_length / 2.0

    for i in range(segments):
        # Angles for current segment and next segment
        theta1 = (i / segments) * 2.0 * math.pi
        theta2 = ((i + 1) / segments) * 2.0 * math.pi

        # U coordinates
        u1 = i / segments
        u2 = (i + 1) / segments

        # Calculate X, Y for current and next
        x1 = radius * math.cos(theta1)
        y1 = radius * math.sin(theta1)
        x2 = radius * math.cos(theta2)
        y2 = radius * math.sin(theta2)

        # Normals (pointing inward = -x, -y, 0)
        n1 = (-x1/radius, -y1/radius, 0.0)
        n2 = (-x2/radius, -y2/radius, 0.0)

        # Define the 4 corners of the quad
        # Near ring (z_start)
        p1_near = (x1, y1, z_start)
        p2_near = (x2, y2, z_start)
        
        # Far ring (z_end)
        p1_far  = (x1, y1, z_end)
        p2_far  = (x2, y2, z_end)

        # UVs
        t1_near = (u1, 0.0)
        t2_near = (u2, 0.0)
        t1_far  = (u1, 1.0)
        t2_far  = (u2, 1.0)

        # Add 2 triangles for the quad (Winding CCW for inward facing)
        # Tri 1: Near1 -> Far1 -> Near2
        add_triangle(p1_near, p1_far, p2_near, t1_near, t1_far, t2_near, n1, n1, n2)
        # Tri 2: Near2 -> Far1 -> Far2
        add_triangle(p2_near, p1_far, p2_far, t2_near, t1_far, t2_far, n2, n1, n2)

    # --- 2. Generate Front Cap (Pyramid/Cone at +Z) ---
    tip_front = (0.0, 0.0, z_end + cap_length)
    # Tip normal? Pointing back towards center roughly, or just -Z
    n_tip_front = (0.0, 0.0, -1.0) 
    t_tip_front = (0.5, 1.0) # Center top of texture

    for i in range(segments):
        theta1 = (i / segments) * 2.0 * math.pi
        theta2 = ((i + 1) / segments) * 2.0 * math.pi
        
        u1 = i / segments
        u2 = (i + 1) / segments

        x1 = radius * math.cos(theta1)
        y1 = radius * math.sin(theta1)
        x2 = radius * math.cos(theta2)
        y2 = radius * math.sin(theta2)

        # Base points on the cylinder edge
        p1 = (x1, y1, z_end)
        p2 = (x2, y2, z_end)

        # Normals for base points (same as cylinder)
        n1 = (-x1/radius, -y1/radius, 0.0)
        n2 = (-x2/radius, -y2/radius, 0.0)

        # UVs
        t1 = (u1, 0.0)
        t2 = (u2, 0.0)

        # Triangle: Base1 -> Base2 -> Tip (Winding for inward)
        # If we look from inside towards +Z, vertices should be CCW.
        add_triangle(p1, p2, tip_front, t1, t2, t_tip_front, n1, n2, n_tip_front)

    # --- 3. Generate Back Cap (Pyramid/Cone at -Z) ---
    tip_back = (0.0, 0.0, z_start - cap_length)
    n_tip_back = (0.0, 0.0, 1.0)
    t_tip_back = (0.5, 1.0)

    for i in range(segments):
        theta1 = (i / segments) * 2.0 * math.pi
        theta2 = ((i + 1) / segments) * 2.0 * math.pi
        
        u1 = i / segments
        u2 = (i + 1) / segments

        x1 = radius * math.cos(theta1)
        y1 = radius * math.sin(theta1)
        x2 = radius * math.cos(theta2)
        y2 = radius * math.sin(theta2)

        # Base points on the cylinder edge
        p1 = (x1, y1, z_start)
        p2 = (x2, y2, z_start)

        n1 = (-x1/radius, -y1/radius, 0.0)
        n2 = (-x2/radius, -y2/radius, 0.0)

        t1 = (u1, 0.0)
        t2 = (u2, 0.0)

        # Triangle: Base2 -> Base1 -> Tip (Reversed winding for back cap)
        add_triangle(p2, p1, tip_back, t2, t1, t_tip_back, n2, n1, n_tip_back)

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
    # Adjust dimensions here
    # radius: width of the tunnel
    # tunnel_length: length of the straight part
    # cap_length: length of the pointy ends
    generate_cigar_mxmod()
