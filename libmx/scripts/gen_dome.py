import math
import sys

def generate_dome_mxmod(filename="dome.mxmod", radius=10.0, slices=32, stacks=16):
    """
    Generates a hemisphere (dome) .mxmod file.
    - Camera is assumed to be at (0,0,0) looking up.
    - Normals point INWARD.
    - Winding is CCW relative to the viewer inside.
    - Y is Up.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # Helper to calculate vertex attributes for a specific grid point
    def get_vertex(stack_i, slice_j):
        # Phi: 0 (Top/Zenith) to Pi/2 (Equator)
        phi = (stack_i / stacks) * (math.pi / 2.0)
        # Theta: 0 to 2*Pi (Around the Y axis)
        theta = (slice_j / slices) * (2.0 * math.pi)

        # Cartesian coordinates (Y is Up)
        # x = r * sin(phi) * cos(theta)
        # z = r * sin(phi) * sin(theta)
        # y = r * cos(phi)
        
        sin_phi = math.sin(phi)
        cos_phi = math.cos(phi)
        sin_theta = math.sin(theta)
        cos_theta = math.cos(theta)

        x = radius * sin_phi * cos_theta
        y = radius * cos_phi
        z = radius * sin_phi * sin_theta

        # Texture Coordinates
        # U: 0 to 1 around the dome
        u = slice_j / slices
        # V: 1.0 at Top (Zenith) to 0.0 at Equator
        v = 1.0 - (stack_i / stacks)

        # Normal (Inward pointing)
        # Since the dome is centered at (0,0,0), the inward normal is just -Position normalized.
        nx = -x / radius
        ny = -y / radius
        nz = -z / radius

        return (x, y, z), (u, v), (nx, ny, nz)

    # --- Generate Geometry ---
    for i in range(stacks):
        for j in range(slices):
            # Calculate the 4 corners of the current quad
            # A(i,j)      D(i,j+1)
            # B(i+1,j)    C(i+1,j+1)
            
            pA, tA, nA = get_vertex(i, j)
            pB, tB, nB = get_vertex(i + 1, j)
            pC, tC, nC = get_vertex(i + 1, j + 1)
            pD, tD, nD = get_vertex(i, j + 1)

            # Winding Order:
            # We are inside looking out. We want Front-Facing triangles (CCW).
            # Standard outward sphere uses A->B->C.
            # For inward facing, we reverse the logic or pick the diagonal that faces in.
            
            # Triangle 1: Top-Left -> Bottom-Left -> Top-Right (A -> B -> D)
            # (This winding produces a normal pointing towards the origin)
            
            # Special case: Stack 0 is the top pole. A and D are the same point.
            # So we skip Triangle 1 for the very top row to avoid degenerate triangles.
            if i > 0:
                vertices.extend([pA, pB, pD])
                tex_coords.extend([tA, tB, tD])
                normals.extend([nA, nB, nD])

            # Triangle 2: Bottom-Left -> Bottom-Right -> Top-Right (B -> C -> D)
            vertices.extend([pB, pC, pD])
            tex_coords.extend([tB, tC, tD])
            normals.extend([nB, nC, nD])

    num_verts = len(vertices)

    # --- Write File ---
    try:
        with open(filename, 'w') as f:
            # Header
            f.write("tri 0 0\n")
            
            # Vertices Block
            f.write(f"vert {num_verts}\n")
            for v in vertices:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n") # Spacer

            # Texture Coordinates Block
            f.write(f"tex {num_verts}\n")
            for t in tex_coords:
                f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            f.write("\n") # Spacer

            # Normals Block
            f.write(f"norm {num_verts}\n")
            for n in normals:
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        
        print(f"Successfully generated {filename} with {num_verts} vertices.")
        
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
	generate_dome_mxmod()
