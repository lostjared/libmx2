import math
import sys

def generate_pillar_mxmod(filename="pillar.mxmod", 
                          radius=10.0, 
                          height=50.0, 
                          segments=32,
                          normals_inward=False):
    """
    Generates a Pillar (Cylinder) .mxmod file.
    - radius: Radius of the circular ends.
    - height: Total height (Y-axis).
    - segments: Number of sides (smoothness).
    - normals_inward: Set to True if you want to be inside the pillar (tunnel).
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # Helper to add a triangle
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        if normals_inward:
            # Flip winding and normals for inside view
            vertices.extend([p1, p3, p2])
            tex_coords.extend([t1, t3, t2])
            normals.extend([(-n1[0], -n1[1], -n1[2]), 
                            (-n3[0], -n3[1], -n3[2]), 
                            (-n2[0], -n2[1], -n2[2])])
        else:
            vertices.extend([p1, p2, p3])
            tex_coords.extend([t1, t2, t3])
            normals.extend([n1, n2, n3])

    # --- Geometry Generation ---
    
    y_top = height / 2.0
    y_bot = -height / 2.0
    
    # 1. Side Faces (Tube)
    for i in range(segments):
        u_curr = i / segments
        u_next = (i + 1) / segments
        
        theta_curr = u_curr * 2.0 * math.pi
        theta_next = u_next * 2.0 * math.pi
        
        # Calculate positions on the circle
        x1 = radius * math.cos(theta_curr)
        z1 = radius * math.sin(theta_curr)
        x2 = radius * math.cos(theta_next)
        z2 = radius * math.sin(theta_next)
        
        # Normals for sides (pointing out from center axis)
        n1 = (math.cos(theta_curr), 0.0, math.sin(theta_curr))
        n2 = (math.cos(theta_next), 0.0, math.sin(theta_next))
        
        # 4 Corners of the quad
        p_tl = (x1, y_top, z1) # Top Left
        p_bl = (x1, y_bot, z1) # Bottom Left
        p_br = (x2, y_bot, z2) # Bottom Right
        p_tr = (x2, y_top, z2) # Top Right
        
        t_tl = (u_curr, 1.0)
        t_bl = (u_curr, 0.0)
        t_br = (u_next, 0.0)
        t_tr = (u_next, 1.0)
        
        # Tri 1: TL -> BL -> BR
        add_triangle(p_tl, p_bl, p_br, t_tl, t_bl, t_br, n1, n1, n2)
        # Tri 2: TL -> BR -> TR
        add_triangle(p_tl, p_br, p_tr, t_tl, t_br, t_tr, n1, n2, n2)

    # 2. Top Cap (Circle)
    center_top = (0.0, y_top, 0.0)
    n_top = (0.0, 1.0, 0.0)
    t_center = (0.5, 0.5)
    
    for i in range(segments):
        u_curr = i / segments
        u_next = (i + 1) / segments
        theta_curr = u_curr * 2.0 * math.pi
        theta_next = u_next * 2.0 * math.pi
        
        x1 = radius * math.cos(theta_curr)
        z1 = radius * math.sin(theta_curr)
        x2 = radius * math.cos(theta_next)
        z2 = radius * math.sin(theta_next)
        
        p_curr = (x1, y_top, z1)
        p_next = (x2, y_top, z2)
        
        # Planar UV mapping
        t_curr = (0.5 + 0.5*math.cos(theta_curr), 0.5 + 0.5*math.sin(theta_curr))
        t_next = (0.5 + 0.5*math.cos(theta_next), 0.5 + 0.5*math.sin(theta_next))
        
        # CCW: Center -> Curr -> Next
        add_triangle(center_top, p_curr, p_next, t_center, t_curr, t_next, n_top, n_top, n_top)

    # 3. Bottom Cap (Circle)
    center_bot = (0.0, y_bot, 0.0)
    n_bot = (0.0, -1.0, 0.0)
    
    for i in range(segments):
        u_curr = i / segments
        u_next = (i + 1) / segments
        theta_curr = u_curr * 2.0 * math.pi
        theta_next = u_next * 2.0 * math.pi
        
        x1 = radius * math.cos(theta_curr)
        z1 = radius * math.sin(theta_curr)
        x2 = radius * math.cos(theta_next)
        z2 = radius * math.sin(theta_next)
        
        p_curr = (x1, y_bot, z1)
        p_next = (x2, y_bot, z2)
        
        t_curr = (0.5 + 0.5*math.cos(theta_curr), 0.5 + 0.5*math.sin(theta_curr))
        t_next = (0.5 + 0.5*math.cos(theta_next), 0.5 + 0.5*math.sin(theta_next))
        
        # CW for bottom: Center -> Next -> Curr
        add_triangle(center_bot, p_next, p_curr, t_center, t_next, t_curr, n_bot, n_bot, n_bot)

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
    # Adjust size here
    generate_pillar_mxmod("pillar.mxmod", radius=10.0, height=50.0)