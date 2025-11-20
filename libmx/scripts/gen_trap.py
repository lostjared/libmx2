import math
import sys

def generate_stacked_trapezoids_mxmod(filename="trapezoid_skybox.mxmod", 
                                      bottom_width=20.0, 
                                      mid_width=15.0, 
                                      top_width=8.0,
                                      bottom_y=-10.0,
                                      mid_y=5.0,
                                      top_y=15.0):
    """
    Generates a skybox shape consisting of two stacked trapezoidal sections.
    - 4-sided (Square base) to create distinct trapezoid faces.
    - Open bottom.
    - Closed top (Cap).
    - Normals point INWARD.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # --- Helper Functions ---
    def add_triangle(v1, v2, v3, t1, t2, t3):
        # Calculate Face Normal (Flat Shading)
        # Vector U = v2 - v1
        ux, uy, uz = v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]
        # Vector V = v3 - v1
        vx, vy, vz = v3[0]-v1[0], v3[1]-v1[1], v3[2]-v1[2]
        
        # Cross Product
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        
        # Normalize
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        if length > 0:
            nx, ny, nz = nx/length, ny/length, nz/length
        
        # Append vertices, UVs, and the calculated normal for each vertex
        vertices.extend([v1, v2, v3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([(nx, ny, nz)] * 3)

    def add_quad(p_bl, p_br, p_tr, p_tl, u_min, u_max, v_min, v_max):
        # Adds a quad composed of two triangles.
        # Winding order is assumed to be CCW relative to the desired normal direction.
        t_bl = (u_min, v_min)
        t_br = (u_max, v_min)
        t_tr = (u_max, v_max)
        t_tl = (u_min, v_max)
        
        # Tri 1: BL -> BR -> TR
        add_triangle(p_bl, p_br, p_tr, t_bl, t_br, t_tr)
        # Tri 2: BL -> TR -> TL
        add_triangle(p_bl, p_tr, p_tl, t_bl, t_tr, t_tl)

    # --- Geometry Definition ---
    
    # Half-widths
    hw0 = bottom_width / 2.0
    hw1 = mid_width / 2.0
    hw2 = top_width / 2.0
    
    # Define the 3 rings of vertices (4 corners each for a square profile)
    # Order: Front-Right, Back-Right, Back-Left, Front-Left
    # This corresponds to angles: 45, 315, 225, 135 degrees roughly
    
    # Ring 0: Bottom
    r0 = [
        ( hw0, bottom_y,  hw0), # 0: FR
        ( hw0, bottom_y, -hw0), # 1: BR
        (-hw0, bottom_y, -hw0), # 2: BL
        (-hw0, bottom_y,  hw0)  # 3: FL
    ]
    
    # Ring 1: Middle
    r1 = [
        ( hw1, mid_y,  hw1),
        ( hw1, mid_y, -hw1),
        (-hw1, mid_y, -hw1),
        (-hw1, mid_y,  hw1)
    ]
    
    # Ring 2: Top
    r2 = [
        ( hw2, top_y,  hw2),
        ( hw2, top_y, -hw2),
        (-hw2, top_y, -hw2),
        (-hw2, top_y,  hw2)
    ]

    # --- Generate Faces ---

    # 1. Lower Tier (Bottom to Middle)
    # We iterate through the 4 sides.
    for i in range(4):
        next_i = (i + 1) % 4
        
        # Vertices for this face
        # Note: To face INWARD, we need to wind CCW from the inside perspective.
        # Looking at the Right Face (between FR and BR):
        # Inside, FR is to the Left, BR is to the Right.
        # So Bottom-Left is FR, Bottom-Right is BR.
        
        p_bot_curr = r0[i]      # Bottom Left
        p_bot_next = r0[next_i] # Bottom Right
        p_mid_next = r1[next_i] # Top Right
        p_mid_curr = r1[i]      # Top Left
        
        u_min = i / 4.0
        u_max = (i + 1) / 4.0
        
        add_quad(p_bot_curr, p_bot_next, p_mid_next, p_mid_curr, u_min, u_max, 0.0, 0.5)

    # 2. Upper Tier (Middle to Top)
    for i in range(4):
        next_i = (i + 1) % 4
        
        p_mid_curr = r1[i]
        p_mid_next = r1[next_i]
        p_top_next = r2[next_i]
        p_top_curr = r2[i]
        
        u_min = i / 4.0
        u_max = (i + 1) / 4.0
        
        add_quad(p_mid_curr, p_mid_next, p_top_next, p_top_curr, u_min, u_max, 0.5, 1.0)

    # 3. Top Cap (Ceiling)
    # Closing the top so it works as a skybox.
    # Winding CCW looking UP: FR -> FL -> BL -> BR
    # Indices in r2: 0 -> 3 -> 2 -> 1
    
    p_fr = r2[0]
    p_br = r2[1]
    p_bl = r2[2]
    p_fl = r2[3]
    
    # UVs for cap (simple planar map)
    t_fr = (1.0, 1.0)
    t_br = (1.0, 0.0)
    t_bl = (0.0, 0.0)
    t_fl = (0.0, 1.0)
    
    # Tri 1: FR -> FL -> BL
    add_triangle(p_fr, p_fl, p_bl, t_fr, t_fl, t_bl)
    # Tri 2: FR -> BL -> BR
    add_triangle(p_fr, p_bl, p_br, t_fr, t_bl, t_br)

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
    # You can adjust the shape here
    generate_stacked_trapezoids_mxmod("trapezoid_skybox.mxmod", 
                                      bottom_width=30.0, 
                                      mid_width=20.0)
