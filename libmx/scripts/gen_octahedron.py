import math
import sys

def generate_octahedron_mxmod(filename="octahedron_large.mxmod", 
                              radius=40.0, 
                              normals_inward=True):
    """
    Generates a Large Octahedron (8 triangular faces).
    - radius: Distance from center to vertices.
    - normals_inward: True for Skybox (camera inside), False for Object.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # Helper to add a triangle
    def add_triangle(p1, p2, p3, t1, t2, t3):
        # Calculate Normal
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l > 0: nx, ny, nz = nx/l, ny/l, nz/l
        
        # Check direction relative to center (0,0,0)
        # Centroid of triangle
        cx = (p1[0]+p2[0]+p3[0])/3.0
        cy = (p1[1]+p2[1]+p3[1])/3.0
        cz = (p1[2]+p2[2]+p3[2])/3.0
        
        # Dot product of Normal and Centroid vector
        dot = nx*cx + ny*cy + nz*cz
        
        # If normals_inward is True, we want Normal pointing opposite to Centroid vector (Dot < 0)
        # If normals_inward is False, we want Normal pointing same as Centroid vector (Dot > 0)
        
        flip = False
        if normals_inward:
            if dot > 0: flip = True
        else:
            if dot < 0: flip = True
            
        if flip:
            nx, ny, nz = -nx, -ny, -nz
            # Swap winding
            p1, p3 = p3, p1
            t1, t3 = t3, t1

        vertices.extend([p1, p2, p3])
        tex_coords.extend([t1, t2, t3])
        normals.extend([(nx, ny, nz)] * 3)

    # --- Define 6 Vertices of Octahedron ---
    # Top, Bottom, Front, Back, Left, Right
    top = (0.0, radius, 0.0)
    bot = (0.0, -radius, 0.0)
    
    front = (0.0, 0.0, radius)
    back  = (0.0, 0.0, -radius)
    left  = (-radius, 0.0, 0.0)
    right = (radius, 0.0, 0.0)
    
    # UV Coordinates (Simple mapping)
    t_top = (0.5, 1.0)
    t_bot = (0.5, 0.0)
    t_left = (0.0, 0.5)
    t_right = (1.0, 0.5)
    t_center = (0.5, 0.5) # Approximate for equator vertices

    # --- Generate 8 Faces ---
    
    # Top Hemisphere (4 faces)
    add_triangle(top, front, right, t_top, t_left, t_right)
    add_triangle(top, right, back,  t_top, t_left, t_right)
    add_triangle(top, back,  left,  t_top, t_left, t_right)
    add_triangle(top, left,  front, t_top, t_left, t_right)
    
    # Bottom Hemisphere (4 faces)
    add_triangle(bot, right, front, t_bot, t_right, t_left)
    add_triangle(bot, back,  right, t_bot, t_right, t_left)
    add_triangle(bot, left,  back,  t_bot, t_right, t_left)
    add_triangle(bot, front, left,  t_bot, t_right, t_left)

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
    # radius=40.0 is 4x larger than a standard 10.0 unit
    generate_octahedron_mxmod(radius=40.0)
