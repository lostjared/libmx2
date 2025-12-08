import math
import sys

# --- Helper Functions for 3D Math ---
def normalize_vector(v):
    """Calculates unit vector to determine direction."""
    msg = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if msg == 0: return (0.0, 0.0, 1.0) # safe default
    return (v[0]/msg, v[1]/msg, v[2]/msg)

def get_triangle_center(v1, v2, v3):
    """Finds the center point on the face of a triangle."""
    cx = (v1[0] + v2[0] + v3[0]) / 3.0
    cy = (v1[1] + v2[1] + v3[1]) / 3.0
    cz = (v1[2] + v2[2] + v3[2]) / 3.0
    return (cx, cy, cz)

def generate_enclosed_kaleidoscope(filename="kaleidoscope_full.mxmod"):
    # --- CONFIGURATION ---
    SIDES = 6              # 6 = Hexagon (Best for kaleidoscope)
    RADIUS = 50.0          # Width of the main tube
    PRISM_HEIGHT = 150.0   # Height of the straight middle section
    CAP_HEIGHT = 60.0      # Height of the pyramids at the ends
    
    # UV Configuration for "Infinite Reflection" effect.
    # We split the texture vertically. 
    # Middle part goes on sides, top/bottom parts stretch onto pyramids.
    V_BOT_TIP = 0.0
    V_PRISM_BOT = 0.2
    V_PRISM_TOP = 0.8
    V_TOP_TIP = 1.0

    # --- Geometry Z-Levels ---
    z_prism_top = PRISM_HEIGHT / 2.0
    z_prism_bot = -PRISM_HEIGHT / 2.0
    # The tips are further out along Z axes
    z_peak_top = z_prism_top + CAP_HEIGHT
    z_peak_bot = z_prism_bot - CAP_HEIGHT

    # The single central vertices for the pyramid tips
    peak_top_v = (0.0, 0.0, z_peak_top)
    peak_bot_v = (0.0, 0.0, z_peak_bot)

    out_verts = []
    out_uvs = []
    out_norms = []

    angle_step = (2 * math.pi) / SIDES

    print(f"Generating {SIDES}-sided enclosed kaleidoscope...")

    for i in range(SIDES):
        # Angles for current segment
        theta1 = i * angle_step
        theta2 = (i + 1) * angle_step

        # Calculate X,Y coordinates on the ring bounds
        x1 = RADIUS * math.cos(theta1)
        y1 = RADIUS * math.sin(theta1)
        x2 = RADIUS * math.cos(theta2)
        y2 = RADIUS * math.sin(theta2)

        # Define the 4 corners of the prism section
        v_bl = (x1, y1, z_prism_bot) # Bottom Left on ring
        v_br = (x2, y2, z_prism_bot) # Bottom Right on ring
        v_tr = (x2, y2, z_prism_top) # Top Right on ring
        v_tl = (x1, y1, z_prism_top) # Top Left on ring

        # --- UV Mirroring Logic ---
        # Flip horizontal U coordinates on every other panel
        is_even = (i % 2 == 0)
        u_left = 0.0 if is_even else 1.0
        u_right = 1.0 if is_even else 0.0
        # The pyramid tip texture coordinate is exactly between left and right
        u_mid = 0.5 

        # =========================================
        # 1. Generate Central Prism Walls
        # =========================================
        # Normal points straight inward to center axis
        mid_theta = (theta1 + theta2) / 2.0
        side_norm = (-math.cos(mid_theta), -math.sin(mid_theta), 0.0)

        # UVs use the middle band of the image (V 0.2 to 0.8)
        uv_bl = (u_left, V_PRISM_BOT)
        uv_br = (u_right, V_PRISM_BOT)
        uv_tr = (u_right, V_PRISM_TOP)
        uv_tl = (u_left, V_PRISM_TOP)

        # Two triangles for the quad wall
        # Tri 1: BL -> BR -> TR
        out_verts.extend([v_bl, v_br, v_tr])
        out_uvs.extend([uv_bl, uv_br, uv_tr])
        out_norms.extend([side_norm]*3)
        # Tri 2: BL -> TR -> TL
        out_verts.extend([v_bl, v_tr, v_tl])
        out_uvs.extend([uv_bl, uv_tr, uv_tl])
        out_norms.extend([side_norm]*3)

        # =========================================
        # 2. Generate Top Pyramid Cap Triangle
        # =========================================
        # Vertices: Top-Left on ring -> Top-Right on ring -> Peak Tip
        
        # Calculate slanted normal pointing inward:
        # Find center of this triangle, vector is from center towards (0,0,0)
        face_center_top = get_triangle_center(v_tl, v_tr, peak_top_v)
        # Invert center coordinate to get direction to origin
        vec_to_center = (-face_center_top[0], -face_center_top[1], -face_center_top[2])
        norm_top = normalize_vector(vec_to_center)

        # UVs stretch from prism top (V=0.8) to image top edge (V=1.0)
        uv_cap_tl = (u_left, V_PRISM_TOP)
        uv_cap_tr = (u_right, V_PRISM_TOP)
        uv_cap_peak = (u_mid, V_TOP_TIP)

        out_verts.extend([v_tl, v_tr, peak_top_v])
        out_uvs.extend([uv_cap_tl, uv_cap_tr, uv_cap_peak])
        out_norms.extend([norm_top]*3)

        # =========================================
        # 3. Generate Bottom Pyramid Cap Triangle
        # =========================================
        # Vertices: Bottom-Right on ring -> Bottom-Left on ring -> Peak Bottom
        
        # Calculate slanted normal pointing inward
        face_center_bot = get_triangle_center(v_br, v_bl, peak_bot_v)
        vec_to_center_bot = (-face_center_bot[0], -face_center_bot[1], -face_center_bot[2])
        norm_bot = normalize_vector(vec_to_center_bot)

        # UVs stretch from prism bottom (V=0.2) to image bottom edge (V=0.0)
        uv_cap_bl = (u_left, V_PRISM_BOT)
        uv_cap_br = (u_right, V_PRISM_BOT)
        uv_cap_peak_bot = (u_mid, V_BOT_TIP)

        out_verts.extend([v_br, v_bl, peak_bot_v])
        out_uvs.extend([uv_cap_br, uv_cap_bl, uv_cap_peak_bot])
        out_norms.extend([norm_bot]*3)


    # --- WRITE MXMOD FILE ---
    vertex_count = len(out_verts)
    
    try:
        with open(filename, "w") as f:
            # Header
            f.write("tri 0 0\n")
            
            # Vertices
            f.write(f"vert {vertex_count}\n")
            for v in out_verts:
                f.write(f"{v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
            f.write("\n")
            
            # Textures
            f.write(f"tex {vertex_count}\n")
            for uv in out_uvs:
                f.write(f"{uv[0]:.4f} {uv[1]:.4f}\n")
            f.write("\n")
            
            # Normals
            f.write(f"norm {vertex_count}\n")
            for n in out_norms:
                f.write(f"{n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")
                
        print(f"Success. Saved to {filename}")
        print(f"Total vertices: {vertex_count}")
        
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    # You can change the filename here
    generate_enclosed_kaleidoscope("kaleidoscope_sealed.mxmod")