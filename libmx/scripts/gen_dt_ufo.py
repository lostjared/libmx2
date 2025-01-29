import math

def generate_circle_vertices(radius, y, segments):
    vertices = []
    # Generate with one extra vertex to close the circle
    for i in range(segments + 1):
        theta = 2 * math.pi * i / segments
        x = radius * math.cos(theta)
        z = radius * math.sin(theta)
        vertices.append((x, y, z))
    return vertices

def generate_ufo_model():
    segments = 64  # Increased for smoothness
    radius = 2.0
    dome_height = 0.8
    ring_radius = 1.1
    ring_height = 0.15

    parts = []

    # Generate Lower Disk (GL_TRIANGLE_FAN)
    circle_vertices = generate_circle_vertices(radius, 0.0, segments)
    
    lower_vertices = [(0.0, 0.0, 0.0)]  # Center point
    lower_vertices.extend(circle_vertices)
    
    lower_texcoords = [(0.5, 0.5)]  # Center UV
    lower_texcoords.extend([
        (0.5 + 0.5 * math.cos(theta), 0.5 + 0.5 * math.sin(theta))
        for theta in (2 * math.pi * i/segments for i in range(segments + 1))
    ])
    
    lower_normals = [(0.0, -1.0, 0.0)] * (len(lower_vertices))

    # Generate Upper Dome (GL_TRIANGLE_FAN)
    dome_vertices = [(0.0, dome_height, 0.0)]  # Top point
    dome_vertices.extend(circle_vertices)
    
    dome_texcoords = [(0.5, 1.0)]  # Top UV
    dome_texcoords.extend([
        (0.5 + 0.5 * v[0]/radius, 0.5 + 0.5 * v[2]/radius)
        for v in circle_vertices
    ])
    
    dome_normals = []
    for v in dome_vertices:
        if v[1] == dome_height:  # Top point
            dome_normals.append((0.0, 1.0, 0.0))
        else:
            length = math.sqrt(v[0]**2 + dome_height**2 + v[2]**2)
            dome_normals.append((
                v[0]/length,
                dome_height/length,
                v[2]/length
            ))

    # Generate Middle Ring (GL_TRIANGLES)
    ring_vertices = []
    ring_texcoords = []
    ring_normals = []
    
    top_ring = generate_circle_vertices(ring_radius, ring_height/2, segments)
    bottom_ring = generate_circle_vertices(ring_radius, -ring_height/2, segments)
    
    for i in range(segments):
        i_next = (i+1) % (segments + 1)  # Proper wrapping
        
        # Quad vertices (two triangles)
        v0 = bottom_ring[i]
        v1 = top_ring[i]
        v2 = bottom_ring[i_next]
        v3 = top_ring[i_next]
        
        # First triangle (v0, v1, v2)
        ring_vertices.extend([v0, v1, v2])
        # Second triangle (v2, v1, v3)
        ring_vertices.extend([v2, v1, v3])
        
        # Texture coordinates
        u_curr = i/segments
        u_next = (i+1)/segments
        ring_texcoords.extend([
            (u_curr, 0), (u_curr, 1), (u_next, 0),
            (u_next, 0), (u_curr, 1), (u_next, 1)
        ])
        
        # Normals (horizontal outward)
        normal = (v0[0]/ring_radius, 0.0, v0[2]/ring_radius)
        ring_normals.extend([normal]*6)

    # Create parts list
    parts.append({
        'mode': 2,  # GL_TRIANGLE_FAN
        'verts': lower_vertices,
        'tex': lower_texcoords,
        'norm': lower_normals
    })

    parts.append({
        'mode': 2,  # GL_TRIANGLE_FAN
        'verts': dome_vertices,
        'tex': dome_texcoords,
        'norm': dome_normals
    })

    parts.append({
        'mode': 0,  # GL_TRIANGLES
        'verts': ring_vertices,
        'tex': ring_texcoords,
        'norm': ring_normals
    })

    # Write to file
    with open('ufo_model_complete.txt', 'w') as f:
        for part in parts:
            f.write(f"tri {part['mode']} 0\n")
            f.write(f"vert {len(part['verts'])}\n")
            for v in part['verts']:
                f.write(f"{v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
            f.write(f"tex {len(part['tex'])}\n")
            for t in part['tex']:
                f.write(f"{t[0]:.4f} {t[1]:.4f}\n")
            f.write(f"norm {len(part['norm'])}\n")
            for n in part['norm']:
                f.write(f"{n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")

if __name__ == "__main__":
    generate_ufo_model()