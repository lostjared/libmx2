import math
import sys

def generate_pentagon_prism_mxmod(filename="pentagon.mxmod", radius=1.0, height=2.0):
    """
    Generates a Pentagonal Prism (5 rectangular sides + Top/Bottom caps).
    Format: .mxmod
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # --- Helper: Add Triangle ---
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([n1, n2, n3])

    # --- Helper: Calculate Normal ---
    def calculate_face_normal(p1, p2, p3):
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l == 0: return (0, 1, 0)
        return (nx/l, ny/l, nz/l)

    # --- 1. Calculate Base Vertices (The Pentagon Rings) ---
    # We need 5 points for the top ring and 5 for the bottom ring.
    top_ring = []
    bot_ring = []
    
    sides = 5
    y_top = height / 2.0
    y_bot = -height / 2.0

    # Start angle at -90 (or similar) to align flat face if desired, 
    # but 0 starts at +X axis.
    for i in range(sides):
        angle = (i / sides) * 2.0 * math.pi
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        
        top_ring.append((x, y_top, z))
        bot_ring.append((x, y_bot, z))

    # --- 2. Generate the 5 Rectangular (Quad) Sides ---
    for i in range(sides):
        next_i = (i + 1) % sides
        
        # Define the 4 corners of the side face
        # Top-Left, Bottom-Left, Bottom-Right, Top-Right (looking from outside)
        p_tl = top_ring[i]
        p_bl = bot_ring[i]
        p_br = bot_ring[next_i]
        p_tr = top_ring[next_i]
        
        # UVs (Standard 0-1 mapping per face)
        t_tl = (0.0, 1.0)
        t_bl = (0.0, 0.0)
        t_br = (1.0, 0.0)
        t_tr = (1.0, 1.0)
        
        # Calculate Normal for this flat face
        # Using TL, BL, BR to determine direction
        n = calculate_face_normal(p_tl, p_bl, p_br)
        
        # Triangle 1: TL -> BL -> BR
        add_triangle(p_tl, p_bl, p_br, t_tl, t_bl, t_br, n, n, n)
        
        # Triangle 2: TL -> BR -> TR
        add_triangle(p_tl, p_br, p_tr, t_tl, t_br, t_tr, n, n, n)

    # --- 3. Generate Top Cap (Pentagon) ---
    # Triangle Fan from center
    center_top = (0.0, y_top, 0.0)
    n_top = (0.0, 1.0, 0.0)
    t_center = (0.5, 0.5) # UV center
    
    for i in range(sides):
        next_i = (i + 1) % sides
        p_curr = top_ring[i]
        p_next = top_ring[next_i]
        
        # Map UV based on position (planar projection)
        t_curr = (0.5 + p_curr[0]/(2*radius), 0.5 + p_curr[2]/(2*radius))
        t_next = (0.5 + p_next[0]/(2*radius), 0.5 + p_next[2]/(2*radius))
        
        # Winding CCW: Center -> Curr -> Next
        add_triangle(center_top, p_curr, p_next, t_center, t_curr, t_next, n_top, n_top, n_top)

    # --- 4. Generate Bottom Cap (Pentagon) ---
    center_bot = (0.0, y_bot, 0.0)
    n_bot = (0.0, -1.0, 0.0)
    
    for i in range(sides):
        next_i = (i + 1) % sides
        p_curr = bot_ring[i]
        p_next = bot_ring[next_i]
        
        t_curr = (0.5 + p_curr[0]/(2*radius), 0.5 + p_curr[2]/(2*radius))
        t_next = (0.5 + p_next[0]/(2*radius), 0.5 + p_next[2]/(2*radius))
        
        # Winding CW (to face down): Center -> Next -> Curr
        add_triangle(center_bot, p_next, p_curr, t_center, t_next, t_curr, n_bot, n_bot, n_bot)

    # --- 5. Write File ---
    num_verts = len(vertices)
    try:
        with open(filename, 'w') as f:
            # Header
            f.write("tri 0 0\n")
            
            # Vertices
            f.write(f"vert {num_verts}\n")
            for v in vertices:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n")

            # Texture Coords
            f.write(f"tex {num_verts}\n")
            for t in tex_coords:
                f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            f.write("\n")

            # Normals
            f.write(f"norm {num_verts}\n")
            for n in normals:
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        
        print(f"Successfully generated {filename} with {num_verts} vertices.")

    except IOError as e:
        print(f"Error writing file: {e}")

if  __name__ == "__main__":
    generate_pentagon_prism_mxmod()