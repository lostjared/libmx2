import math
import sys

def generate_star_mxmod(filename="star_skybox.mxmod", 
                        base_radius=10.0, 
                        spike_height=8.0, 
                        spikes=5, 
                        slices=64, 
                        stacks=32):
    """
    Generates a parametric 3D star (spiky sphere) for a skybox.
    - Camera inside (0,0,0).
    - Normals INWARD.
    - Uses flat shading (duplicated vertices) to make the star facets look crystalline.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # --- Helper: Add Triangle with Auto-Normal ---
    def add_triangle(p1, p2, p3, t1, t2, t3):
        # 1. Calculate Vector U and V
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        
        # 2. Cross Product to get Normal
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        
        # 3. Normalize
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        if length > 0:
            nx, ny, nz = nx/length, ny/length, nz/length
        else:
            nx, ny, nz = 0, 1, 0

        # 4. Check Direction (We want INWARD)
        # Vector from origin to p1 is just p1.
        # Dot product of Normal and Position.
        # If N . P > 0, the normal points same direction as P (Outward).
        # We want Inward, so we flip if it's Outward.
        dot = nx*p1[0] + ny*p1[1] + nz*p1[2]
        
        if dot > 0:
            # Normal is Outward. Flip it.
            nx, ny, nz = -nx, -ny, -nz
            # Also swap winding order (p2 <-> p3) so backface culling works for inside view
            p2, p3 = p3, p2
            t2, t3 = t3, t2

        # 5. Append Data
        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([(nx, ny, nz)] * 3) # Flat shading: same normal for all 3 verts

    # --- Helper: Calculate Star Vertex ---
    def get_star_vertex(row, col):
        # Normalized coordinates (0.0 to 1.0)
        u = col / slices
        v = row / stacks
        
        # Angles
        phi = v * math.pi       # 0 to Pi (Top to Bottom)
        theta = u * 2.0 * math.pi # 0 to 2Pi (Around)

        # --- Star Shape Modulation ---
        # We modulate the radius based on angles to create spikes.
        # Using power of 2 makes the spikes sharper and valleys wider.
        modulation = abs(math.sin(spikes * phi) * math.sin(spikes * theta))
        modulation = math.pow(modulation, 2) 
        
        r = base_radius + (spike_height * modulation)

        # Cartesian conversion (Y is Up)
        x = r * math.sin(phi) * math.cos(theta)
        y = r * math.cos(phi)
        z = r * math.sin(phi) * math.sin(theta)
        
        # UVs (Simple spherical projection)
        # We flip V so texture isn't upside down
        return (x, y, z), (u, 1.0 - v)

    # --- Generate Geometry ---
    for i in range(stacks):
        for j in range(slices):
            # Get 4 corners of the patch
            pA, tA = get_star_vertex(i, j)
            pB, tB = get_star_vertex(i + 1, j)
            pC, tC = get_star_vertex(i + 1, j + 1)
            pD, tD = get_star_vertex(i, j + 1)

            # Create 2 triangles for the quad
            # We pass them in a standard order; add_triangle will flip winding if needed for inward facing.
            add_triangle(pA, pB, pD, tA, tB, tD)
            add_triangle(pB, pC, pD, tB, tC, tD)

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
    # Adjust parameters to change the star shape
    generate_star_mxmod("star_skybox.mxmod", 
                        base_radius=15.0, 
                        spike_height=15.0, 
                        spikes=5, 
                        slices=64)
