import math

def generate_hexagonal_prism_skybox(radius, height):
    """
    Generates the vertex, texture, and normal data for an inward-facing
    hexagonal prism, suitable for a skybox in the MXMOD format.

    The prism is centered at (0, 0, 0).

    Args:
        radius (float): The distance from the center to a vertex on the hexagon face.
        height (float): The total height of the prism (from bottom to top face).

    Returns:
        tuple: (vert_data, tex_data, norm_data)
    """
    # 1. Define the 6 vertices of the top and bottom faces
    # The hexagon starts at an angle of 30 degrees (pi/6) for a "pointy-top" look
    # or 0 degrees for a "flat-top" look. We'll use 0 degrees for flat-top
    # which aligns faces with the X, Y axes more nicely.
    num_sides = 6
    half_h = height / 2.0
    vert_face = []

    # Calculate the 6 vertices on the XY plane for a flat-top hexagon
    for i in range(num_sides):
        angle = 2 * math.pi * i / num_sides
        x = radius * math.cos(angle)
        y = radius * math.sin(angle)
        vert_face.append((x, y))

    # 2. Generate side faces (6 rectangles, 12 triangles)
    vert_data = []
    tex_data = []
    norm_data = []
    
    # 2.1 Side Faces (6 quads -> 12 triangles -> 36 vertices total for sides)
    for i in range(num_sides):
        # Current and next vertex indices on the face
        v1_xy = vert_face[i]
        v2_xy = vert_face[(i + 1) % num_sides]

        # Vertices for the quad (bottom, top, top, bottom)
        # Quad: v1_b, v2_b, v2_t, v1_t
        v1_b = (v1_xy[0], v1_xy[1], -half_h) # 0
        v2_b = (v2_xy[0], v2_xy[1], -half_h) # 1
        v2_t = (v2_xy[0], v2_xy[1], half_h)  # 2
        v1_t = (v1_xy[0], v1_xy[1], half_h)  # 3

        # Inward-facing Normal calculation
        # Vector from center (0,0) to midpoint of the face edge on the XY plane
        mid_x = (v1_xy[0] + v2_xy[0]) / 2.0
        mid_y = (v1_xy[1] + v2_xy[1]) / 2.0
        # Normalize the vector pointing *towards* the center (inward)
        length = math.sqrt(mid_x**2 + mid_y**2)
        if length == 0:
            nx, ny = 0.0, 0.0
        else:
            nx = -mid_x / length
            ny = -mid_y / length

        normal = (nx, ny, 0.0)

        # Triangle 1 (T1): v1_b, v2_t, v1_t (Wound CCW from inside)
        # This forms the first triangle (v1_b, v2_b, v2_t) of the quad, but reordered for inward face
        tri1_verts = [v1_b, v2_t, v1_t]
        tri1_texs = [(0.0, 0.0), (1.0, 1.0), (0.0, 1.0)]

        # Triangle 2 (T2): v1_b, v2_b, v2_t (Wound CCW from inside)
        # Tri 2: v1_b, v2_b, v2_t (This closes the quad: v1_t, v2_t, v1_b) - Let's stick to v1_b, v2_b, v2_t, then v1_b, v2_t, v1_t for a standard quad split
        # To get the second triangle v1_b, v2_b, v2_t, let's use:
        # T2: v1_b, v2_b, v2_t (Wound CCW from inside) -> This is wrong. It should be v1_t, v2_b, v1_b.
        # Let's use the standard quad split and winding for inward normals:
        # Quad (v1_b, v2_b, v2_t, v1_t)
        
        # Tri 1: v1_b, v2_b, v2_t (Bottom-Left, Bottom-Right, Top-Right)
        vert_data.extend([v1_b, v2_b, v2_t])
        tex_data.extend([(0.0, 0.0), (1.0, 0.0), (1.0, 1.0)])
        norm_data.extend([normal, normal, normal])

        # Tri 2: v1_b, v2_t, v1_t (Bottom-Left, Top-Right, Top-Left)
        vert_data.extend([v1_b, v2_t, v1_t])
        tex_data.extend([(0.0, 0.0), (1.0, 1.0), (0.0, 1.0)])
        norm_data.extend([normal, normal, normal])


    # 2.2 Top Face (4 triangles -> 12 vertices total for top)
    # The center point of the top face
    center_t = (0.0, 0.0, half_h)
    # Inward normal for top face is (0, 0, -1)
    norm_t = (0.0, 0.0, -1.0)
    
    # We use a fan starting from the center and going around the perimeter
    # Since it's a hexagon, we can split it into 4 triangles for a cleaner mesh,
    # or 6 triangles for a center-fan method. We will use the 6 triangle fan method.
    for i in range(num_sides):
        v_curr = (vert_face[i][0], vert_face[i][1], half_h)
        v_next = (vert_face[(i + 1) % num_sides][0], vert_face[(i + 1) % num_sides][1], half_h)
        
        # Triangles are wound inward: Center, Next, Current
        vert_data.extend([center_t, v_next, v_curr])
        
        # Texture coordinates for a fan: center is (0.5, 0.5), perimeter points
        # are calculated based on their angle relative to the center.
        tex_data.extend([(0.5, 0.5), (0.5 + 0.5 * math.cos(2 * math.pi * (i + 1) / num_sides), 0.5 + 0.5 * math.sin(2 * math.pi * (i + 1) / num_sides)), (0.5 + 0.5 * math.cos(2 * math.pi * i / num_sides), 0.5 + 0.5 * math.sin(2 * math.pi * i / num_sides))])
        norm_data.extend([norm_t, norm_t, norm_t])

    # 2.3 Bottom Face (6 triangles -> 18 vertices total for bottom)
    # The center point of the bottom face
    center_b = (0.0, 0.0, -half_h)
    # Inward normal for bottom face is (0, 0, 1)
    norm_b = (0.0, 0.0, 1.0)

    # Fan starting from the center
    for i in range(num_sides):
        v_curr = (vert_face[i][0], vert_face[i][1], -half_h)
        v_next = (vert_face[(i + 1) % num_sides][0], vert_face[(i + 1) % num_sides][1], -half_h)
        
        # Triangles are wound inward: Center, Current, Next (opposite of top)
        vert_data.extend([center_b, v_curr, v_next])
        
        # Texture coordinates for a fan
        tex_data.extend([(0.5, 0.5), (0.5 + 0.5 * math.cos(2 * math.pi * i / num_sides), 0.5 + 0.5 * math.sin(2 * math.pi * i / num_sides)), (0.5 + 0.5 * math.cos(2 * math.pi * (i + 1) / num_sides), 0.5 + 0.5 * math.sin(2 * math.pi * (i + 1) / num_sides))])
        norm_data.extend([norm_b, norm_b, norm_b])
        
    return vert_data, tex_data, norm_data

def format_mxmod(vert_data, tex_data, norm_data):
    """
    Formats the data into the MXMOD file layout.
    """
    total_verts = len(vert_data)
    output = f"tri 0 0\nvert {total_verts}\n"
    
    # Vertices
    for v in vert_data:
        output += f"{v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n"
        
    # Texture Coordinates
    output += f"\ntex {total_verts}\n"
    for t in tex_data:
        output += f"{t[0]:.4f} {t[1]:.4f}\n"
        
    # Normals
    output += f"\nnorm {total_verts}\n"
    for n in norm_data:
        output += f"{n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n"
        
    return output.strip()

# --- Configuration ---
# Radius of the hexagonal face (distance from center to vertex)
R = 100.0 
# Total height of the prism
H = 100.0

# Generate the data
V, T, N = generate_hexagonal_prism_skybox(radius=R, height=H)

# Format the output
mxmod_output = format_mxmod(V, T, N)

# Print the result
print(mxmod_output)

# Total faces: 6 sides + 1 top + 1 bottom = 8 faces
# Total triangles: (6 sides * 2 tris/side) + (2 ends * 6 tris/end) = 12 + 12 = 24 triangles
# Total vertices: 24 triangles * 3 vertices/triangle = 72 vertices
print(f"\n# Total Vertices Generated: {len(V)}")