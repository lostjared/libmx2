import math

# --- CONFIGURATION ---
FILENAME = "hourglass_skybox.mxmod"

# Dimensions
TOTAL_HEIGHT = 40.0    # How long the tunnel is (from Y=-20 to Y=20)
WAIST_RADIUS = 2.0     # The narrowest point in the middle
END_RADIUS = 15.0      # The widest point at the top/bottom openings

# Resolution (Higher = smoother curve)
RINGS_VERTICAL = 64    # Needs many rings to make the curve look smooth
SEGMENTS_AROUND = 32   # How round the tube is

def generate_file():
    vertices = []
    uvs = []
    normals = []

    def add_quad_inward(v1, v2, v3, v4):
        # Adds a quad split into two triangles.
        # Winding order reversed (v4->v3->v2->v1) so faces point INWARD.
        # Tri 1
        vertices.extend([v4['pos'], v3['pos'], v2['pos']])
        uvs.extend([v4['uv'], v3['uv'], v2['uv']])
        normals.extend([v4['norm'], v3['norm'], v2['norm']])
        # Tri 2
        vertices.extend([v4['pos'], v2['pos'], v1['pos']])
        uvs.extend([v4['uv'], v2['uv'], v1['uv']])
        normals.extend([v4['norm'], v2['norm'], v1['norm']])

    # Helper function to calculate radius at a specific Y height using a curve
    def get_radius_at_y(y_pos):
        # Normalize Y from range [-HEIGHT/2, HEIGHT/2] to range [0, 1] representing distance from center
        # 0 = at center, 1 = at ends
        half_height = TOTAL_HEIGHT / 2.0
        dist_from_center_norm = abs(y_pos) / half_height
        
        # Use a power curve (square) to make the pinch smooth.
        # Radius = Waist + (Difference * distance^2)
        r = WAIST_RADIUS + (END_RADIUS - WAIST_RADIUS) * (dist_from_center_norm * dist_from_center_norm)
        return r

    # Generate the geometry
    y_start = -TOTAL_HEIGHT / 2.0

    for r in range(RINGS_VERTICAL):
        # Vertical indices
        curr_r_idx = r
        next_r_idx = r + 1
        
        # Calculate V coordinates (0.0 at bottom, 1.0 at top)
        v_curr = float(curr_r_idx) / RINGS_VERTICAL
        v_next = float(next_r_idx) / RINGS_VERTICAL
        
        # Calculate actual Y positions
        y_curr = y_start + (v_curr * TOTAL_HEIGHT)
        y_next = y_start + (v_next * TOTAL_HEIGHT)
        
        # Calculate Radii at these heights
        rad_curr = get_radius_at_y(y_curr)
        rad_next = get_radius_at_y(y_next)

        for s in range(SEGMENTS_AROUND):
            # Horizontal indices
            curr_s_idx = s
            next_s_idx = (s + 1) % SEGMENTS_AROUND
            
            # Calculate U coordinates (0.0 to 1.0 around)
            u_curr = float(curr_s_idx) / SEGMENTS_AROUND
            u_next = float(next_s_idx) / SEGMENTS_AROUND
            
            # Angles
            theta_curr = 2.0 * math.pi * u_curr
            theta_next = 2.0 * math.pi * u_next
            
            # Helper to generate a vertex dict
            def make_vert(rad, y, theta, u, v):
                x = rad * math.cos(theta)
                z = rad * math.sin(theta)
                # Simplified Normal: Pointing horizontally inward to center axis.
                # This is usually sufficient for abstract skyboxes.
                nx = -math.cos(theta)
                nz = -math.sin(theta)
                return {'pos': (x,y,z), 'uv': (u,v), 'norm': (nx,0,nz)}

            # Define the 4 corners of the patch
            p1 = make_vert(rad_curr, y_curr, theta_curr, u_curr, v_curr)
            p2 = make_vert(rad_next, y_next, theta_curr, u_curr, v_next)
            p3 = make_vert(rad_next, y_next, theta_next, u_next, v_next)
            p4 = make_vert(rad_curr, y_curr, theta_next, u_next, v_curr)
            
            add_quad_inward(p1, p2, p3, p4)


    # WRITE FILE
    with open(FILENAME, 'w') as f:
        count = len(vertices)
        f.write("tri 0 0\n")
        
        f.write(f"vert {count}\n")
        for x,y,z in vertices:
            f.write(f"{x:.6f} {y:.6f} {z:.6f}\n")
        f.write("\n")
            
        f.write(f"tex {count}\n")
        for u,v in uvs:
            f.write(f"{u:.6f} {v:.6f}\n")
        f.write("\n")
            
        f.write(f"norm {count}\n")
        for nx,ny,nz in normals:
            f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")
            
    print(f"Successfully generated {FILENAME} with {count} vertices.")

if __name__ == "__main__":
    generate_file()