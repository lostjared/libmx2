import math
import sys

def generate_uv_sphere_mxmod(filename="uv_sphere.mxmod", 
                             radius=10.0, 
                             slices=32, 
                             stacks=16,
                             normals_inward=True):
    """
    Generates a UV Sphere (Rectangular topology) .mxmod file.
    - Perfect for equirectangular textures.
    - normals_inward=True: Camera inside (Skybox).
    - normals_inward=False: Camera outside (Planet/Ball).
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # Helper to add a triangle
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([n1, n2, n3])

    # --- Generate Vertices ---
    # We generate a grid of vertices.
    # i goes from 0 to stacks (Latitude: Top to Bottom)
    # j goes from 0 to slices (Longitude: Wrap around)
    
    for i in range(stacks):
        for j in range(slices):
            
            # Calculate the 4 corners of the current quad (patch)
            # We need current row (i) and next row (i+1)
            # We need current col (j) and next col (j+1)
            
            # Helper to compute attributes for a specific grid point (row, col)
            def get_attributes(row, col):
                # V: 1.0 at top, 0.0 at bottom
                v = 1.0 - (row / stacks)
                # U: 0.0 to 1.0 around
                u = col / slices
                
                # Phi: 0 to Pi (Top to Bottom)
                phi = (row / stacks) * math.pi
                # Theta: 0 to 2*Pi (Around Y)
                theta = (col / slices) * 2.0 * math.pi
                
                # Spherical to Cartesian
                # y is up
                x = radius * math.sin(phi) * math.cos(theta)
                y = radius * math.cos(phi)
                z = radius * math.sin(phi) * math.sin(theta)
                
                pos = (x, y, z)
                
                # Normal calculation
                # Normalized position vector
                nx = x / radius
                ny = y / radius
                nz = z / radius
                
                if normals_inward:
                    norm = (-nx, -ny, -nz)
                else:
                    norm = (nx, ny, nz)
                    
                uv = (u, v)
                return pos, uv, norm

            # Get the 4 corners
            pA, tA, nA = get_attributes(i, j)         # Top-Left
            pB, tB, nB = get_attributes(i + 1, j)     # Bottom-Left
            pC, tC, nC = get_attributes(i + 1, j + 1) # Bottom-Right
            pD, tD, nD = get_attributes(i, j + 1)     # Top-Right

            # --- Create Triangles ---
            # Winding order depends on normal direction.
            # CCW is standard for Front Facing.
            
            if normals_inward:
                # Inside looking out:
                # Tri 1: A -> B -> D
                add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
                # Tri 2: B -> C -> D
                add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)
            else:
                # Outside looking in:
                # Tri 1: A -> D -> B
                add_triangle(pA, pD, pB, tA, tD, tB, nA, nD, nB)
                # Tri 2: B -> D -> C
                add_triangle(pB, pD, pC, tB, tD, tC, nB, nD, nC)

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
    # Adjust settings here
    # normals_inward=True for Skybox/Inside view
    # normals_inward=False for Object/Outside view
    generate_uv_sphere_mxmod()
