import math
import sys

def generate_sphere_hourglass_mxmod(filename="sphere_hourglass.mxmod", 
                                    sphere_radius=60.0, 
                                    bar_height=100.0, 
                                    bar_waist=3.0, 
                                    bar_cap=12.0,
                                    segments_rad=48, 
                                    segments_height=32,
                                    reverse_winding=False):
    """
    Generates a composite model:
    1. Outer Sphere (Skybox) - Normals Inward.
    2. Central Hourglass Pillar - Normals Outward.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        if reverse_winding:
            vertices.extend([p1, p3, p2])
            tex_coords.extend([t1, t3, t2])
            normals.extend([n1, n3, n2])
        else:
            vertices.extend([p1, p2, p3])
            tex_coords.extend([t1, t2, t3])
            normals.extend([n1, n2, n3])

    # --- 1. Outer Sphere (Skybox) ---
    # Standard UV sphere, normals pointing IN
    for i in range(segments_height):
        v_curr = i / segments_height
        v_next = (i + 1) / segments_height
        
        phi_curr = v_curr * math.pi
        phi_next = v_next * math.pi
        
        for j in range(segments_rad):
            u_curr = j / segments_rad
            u_next = (j + 1) / segments_rad
            
            theta_curr = u_curr * 2.0 * math.pi
            theta_next = u_next * 2.0 * math.pi
            
            def get_sphere_vert(phi, theta):
                x = sphere_radius * math.sin(phi) * math.cos(theta)
                y = sphere_radius * math.cos(phi)
                z = sphere_radius * math.sin(phi) * math.sin(theta)
                # Normal Inward
                nx, ny, nz = -x/sphere_radius, -y/sphere_radius, -z/sphere_radius
                return (x,y,z), (nx,ny,nz)

            pA, nA = get_sphere_vert(phi_curr, theta_curr)
            pB, nB = get_sphere_vert(phi_next, theta_curr)
            pC, nC = get_sphere_vert(phi_next, theta_next)
            pD, nD = get_sphere_vert(phi_curr, theta_next)
            
            tA = (u_curr, v_curr)
            tB = (u_curr, v_next)
            tC = (u_next, v_next)
            tD = (u_next, v_curr)
            
            # CCW from inside
            add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
            add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)

    # --- 2. Central Hourglass (Bar) ---
    # Vertical alignment (Y-axis), Normals pointing OUT
    for i in range(segments_height):
        v_curr = i / segments_height
        v_next = (i + 1) / segments_height
        
        # Map v (0..1) to Y (-h/2 .. h/2)
        y_curr = (v_curr - 0.5) * bar_height
        y_next = (v_next - 0.5) * bar_height
        
        # Calculate Radius based on Y (Parabolic/Hyperbolic shape)
        # Normalized Y from -1 to 1
        ny_curr = (v_curr - 0.5) * 2.0
        ny_next = (v_next - 0.5) * 2.0
        
        # Shape function: r = waist + (cap - waist) * y^2
        r_curr = bar_waist + (bar_cap - bar_waist) * (ny_curr * ny_curr)
        r_next = bar_waist + (bar_cap - bar_waist) * (ny_next * ny_next)
        
        for j in range(segments_rad):
            u_curr = j / segments_rad
            u_next = (j + 1) / segments_rad
            
            theta_curr = u_curr * 2.0 * math.pi
            theta_next = u_next * 2.0 * math.pi
            
            def get_bar_vert(y, r, theta):
                x = r * math.cos(theta)
                z = r * math.sin(theta)
                # Normal approximation (just radial, ignoring slope for simplicity)
                nx, ny, nz = math.cos(theta), 0.0, math.sin(theta)
                return (x, y, z), (nx, ny, nz)

            pA, nA = get_bar_vert(y_curr, r_curr, theta_curr)
            pB, nB = get_bar_vert(y_next, r_next, theta_curr)
            pC, nC = get_bar_vert(y_next, r_next, theta_next)
            pD, nD = get_bar_vert(y_curr, r_curr, theta_next)
            
            tA = (u_curr, v_curr)
            tB = (u_curr, v_next)
            tC = (u_next, v_next)
            tD = (u_next, v_curr)
            
            # CCW from outside
            add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
            add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)

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
    # Set reverse_winding=True if you experience black screen issues
    generate_sphere_hourglass_mxmod()

