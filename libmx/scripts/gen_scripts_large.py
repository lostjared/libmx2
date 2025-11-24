import math
import sys

def generate_large_sphere_mxmod(filename="large_sphere.mxmod", 
                                radius=500.0, 
                                segments_rad=64, 
                                segments_height=32,
                                reverse_winding=False):
    """
    Generates a large UV Sphere for use as a skybox.
    - radius: The size of the sphere (default 500.0).
    - Normals point INWARD.
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

    # --- Generate Sphere Geometry ---
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
                x = radius * math.sin(phi) * math.cos(theta)
                y = radius * math.cos(phi)
                z = radius * math.sin(phi) * math.sin(theta)
                
                # Normal Inward (-x, -y, -z normalized)
                # Since x,y,z are on surface of sphere, just divide by radius
                nx, ny, nz = -x/radius, -y/radius, -z/radius
                return (x,y,z), (nx,ny,nz)

            pA, nA = get_sphere_vert(phi_curr, theta_curr)
            pB, nB = get_sphere_vert(phi_next, theta_curr)
            pC, nC = get_sphere_vert(phi_next, theta_next)
            pD, nD = get_sphere_vert(phi_curr, theta_next)
            
            tA = (u_curr, v_curr)
            tB = (u_curr, v_next)
            tC = (u_next, v_next)
            tD = (u_next, v_curr)
            
            # CCW winding from inside perspective
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
        print(f"Successfully generated {filename} with {num_verts} vertices (Radius: {radius}).")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    # You can change the radius here to whatever size you need
    # e.g. 8 * 60 = 480, or just 500
    generate_large_sphere_mxmod(radius=500.0)