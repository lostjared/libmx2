import math

def cross_product(a, b):
    """Compute cross product of two 3D vectors a and b."""
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )

def normalize(v):
    """Normalize a 3D vector v."""
    length = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if length < 1e-9:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def generate_inverted_star_mxmod_format(
    num_points=5, 
    outer_radius=1.0, 
    inner_radius=0.5, 
    height=1.0
):
    """
    Generate an INVERTED 3D star mesh (polyhedron) in mxmod format.
    The inversion allows the camera to be inside the star and view the faces.
    
    :param num_points:  Number of major points in the star ring.
    :param outer_radius: Radius of outer 'spikes' in the XY plane.
    :param inner_radius: Radius of the inner vertices in the XY plane.
    :param height:       Half the height of the star.
    :return: A string containing the inverted model data in mxmod format.
    """
    
    # 1) Generate star ring vertices in the XY plane
    ring_vertices = []
    angle_step = 2.0 * math.pi / (num_points * 2)
    for i in range(num_points * 2):
        r = outer_radius if i % 2 == 0 else inner_radius
        angle = angle_step * i
        x = r * math.cos(angle)
        y = r * math.sin(angle)
        # We need the vertex definition to be exact here
        ring_vertices.append((x, y, 0.0))

    # 2) Define top and bottom apex
    top_apex = (0.0, 0.0, +height)
    bottom_apex = (0.0, 0.0, -height)

    # 3) Build the triangles
    triangles = []
    n = len(ring_vertices)
    for i in range(n):
        j = (i + 1) % n  # next index (wrap around)
        v1 = ring_vertices[i]
        v2 = ring_vertices[j]

        # --- INVERSION LOGIC ---
        # 1. Top Face: Connects v1, v2, top_apex. 
        #    We reverse the winding order (v2, v1, top_apex) so the face is visible from the inside.
        triangles.append([v2, v1, top_apex])

        # 2. Bottom Face: Connects v2, v1, bottom_apex.
        #    We reverse the winding order (v1, v2, bottom_apex) so the face is visible from the inside.
        triangles.append([v1, v2, bottom_apex])

    # 4) Flatten out all triangles and compute INVERTED normals
    all_verts = []
    all_norms = []
    all_tex = []

    for tri in triangles:
        # tri is [vA, vB, vC]
        vA, vB, vC = tri

        # Compute the normal for the OUTWARD face (using the reversed winding)
        vAB = (vB[0] - vA[0], vB[1] - vA[1], vB[2] - vA[2])
        vAC = (vC[0] - vA[0], vC[1] - vA[1], vC[2] - vA[2])
        
        outward_norm = normalize(cross_product(vAB, vAC))
        
        # INVERT the normal to point INTO the object
        inward_norm = (-outward_norm[0], -outward_norm[1], -outward_norm[2])

        # Store vertices, texture coords, and the INVERTED normal
        for idx, v in enumerate(tri):
            all_verts.append(v)
            all_norms.append(inward_norm)

            # Assign basic UV coords for texture mapping
            if idx == 0:
                all_tex.append((0.0, 0.0))
            elif idx == 1:
                all_tex.append((1.0, 0.0))
            else:  # idx == 2
                all_tex.append((0.5, 1.0))

    # 5) Convert to mxmod-like text
    output_lines = []
    output_lines.append("tri 0 0")

    # Vertices
    output_lines.append(f"vert {len(all_verts)}")
    for vx, vy, vz in all_verts:
        # Use high precision for large coordinates
        output_lines.append(f"{vx:.6f} {vy:.6f} {vz:.6f}")

    # Texture coords
    output_lines.append(f"tex {len(all_tex)}")
    for u, v in all_tex:
        # Texture coordinates remain standard (0.0 to 1.0)
        output_lines.append(f"{u:.4f} {v:.4f}")

    # Normals
    output_lines.append(f"norm {len(all_norms)}")
    for nx, ny, nz in all_norms:
        output_lines.append(f"{nx:.4f} {ny:.4f} {nz:.4f}")

    return "\n".join(output_lines)

def main():
    # --- SKYBOX SCALE PARAMETERS ---
    # Using very large values to ensure the star encompasses the entire scene
    # This creates a star with a width and height of 10,000 units!
    SKYBOX_SCALE = 500.0
    
    # 5-pointed star
    mxmod_data = generate_inverted_star_mxmod_format(
        num_points=5,
        outer_radius=1.0 * SKYBOX_SCALE,  # Spikes out to 5000 units
        inner_radius=0.5 * SKYBOX_SCALE,  # Inner points are at 2500 units
        height=1.0 * SKYBOX_SCALE         # Height up to 5000 units
    )

    OUTPUT_FILENAME = "inverted_star_skybox.mxmod"

    print(f"Generating large, inverted 3D star mesh into {OUTPUT_FILENAME}...")
    
    try:
        with open(OUTPUT_FILENAME, 'w') as f:
            f.write(mxmod_data)
        print(f"Successfully created {OUTPUT_FILENAME}.")
        print("The file contains an inverted 5-pointed star mesh, ready to be used as an inner skybox.")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    main()