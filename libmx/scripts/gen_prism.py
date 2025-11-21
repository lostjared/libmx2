import math
import sys

def generate_prism_mxmod(filename="prism.mxmod", sides=3, radius=1.0, height=2.0):
    """
    Generates a Prism with N sides.
    - sides=3: Triangular Prism
    - sides=6: Hexagonal Prism
    - etc.
    """
    
    if sides < 3:
        print("Error: A prism must have at least 3 sides.")
        return

    vertices = []
    tex_coords = []
    normals = []

    # --- Helper: Add Triangle ---
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([n1, n2, n3])

    # --- Helper: Calculate Face Normal ---
    def calculate_face_normal(p1, p2, p3):
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l == 0: return (0, 1, 0)
        return (nx/l, ny/l, nz/l)

    # --- Pre-calculate Ring Vertices ---
    top_ring = []
    bot_ring = []
    
    y_top = height / 2.0
    y_bot = -height / 2.0

    # Generate points around the circle
    for i in range(sides):
        # Offset angle by -90 degrees (-pi/2) so the first face is flat front if needed, 
        # or just start at 0. Let's start at 0.
        angle = (i / sides) * 2.0 * math.pi
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        
        top_ring.append((x, y_top, z))
        bot_ring.append((x, y_bot, z))

    # --- 1. Generate Side Faces (Quads) ---
    for i in range(sides):
        next_i = (i + 1) % sides
        
        # Corners for the current face
        # CCW winding: TopLeft -> BottomLeft -> BottomRight -> TopRight
        p_tl = top_ring[i]
        p_bl = bot_ring[i]
        p_br = bot_ring[next_i]
        p_tr = top_ring[next_i]
        
        # UVs (0-1 mapping per face)
        t_tl = (0.0, 1.0)
        t_bl = (0.0, 0.0)
        t_br = (1.0, 0.0)
        t_tr = (1.0, 1.0)
        
        # Normal (Flat shading)
        n = calculate_face_normal(p_tl, p_bl, p_br)
        
        # Tri 1: TL -> BL -> BR
        add_triangle(p_tl, p_bl, p_br, t_tl, t_bl, t_br, n, n, n)
        # Tri 2: TL -> BR -> TR
        add_triangle(p_tl, p_br, p_tr, t_tl, t_br, t_tr, n, n, n)

    # --- 2. Generate Top Cap ---
    center_top = (0.0, y_top, 0.0)
    n_top = (0.0, 1.0, 0.0)
    t_center = (0.5, 0.5)
    
    for i in range(sides):
        next_i = (i + 1) % sides
        p_curr = top_ring[i]
        p_next = top_ring[next_i]
        
        # Planar UV mapping
        t_curr = (0.5 + p_curr[0]/(2*radius), 0.5 + p_curr[2]/(2*radius))
        t_next = (0.5 + p_next[0]/(2*radius), 0.5 + p_next[2]/(2*radius))
        
        # Winding CCW (Up): Center -> Curr -> Next
        add_triangle(center_top, p_curr, p_next, t_center, t_curr, t_next, n_top, n_top, n_top)

    # --- 3. Generate Bottom Cap ---
    center_bot = (0.0, y_bot, 0.0)
    n_bot = (0.0, -1.0, 0.0)
    
    for i in range(sides):
        next_i = (i + 1) % sides
        p_curr = bot_ring[i]
        p_next = bot_ring[next_i]
        
        t_curr = (0.5 + p_curr[0]/(2*radius), 0.5 + p_curr[2]/(2*radius))
        t_next = (0.5 + p_next[0]/(2*radius), 0.5 + p_next[2]/(2*radius))
        
        # Winding CW (Down): Center -> Next -> Curr
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
    # Change 'sides' to 3 for a triangle prism, 6 for hexagon, etc.
    generate_prism_mxmod("prism_hex.mxmod", sides=6, radius=1.0)