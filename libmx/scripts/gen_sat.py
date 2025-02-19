import math

def generate_sphere(lat_div, lon_div, radius=1.0):
    """
    Generates the vertices, texture coordinates, and normals for a sphere mesh.
    Uses two triangles per quad defined by adjacent latitude and longitude lines.
    """
    vertices = []
    texcoords = []
    normals = []
    for i in range(lat_div):
        theta1 = math.pi * i / lat_div
        theta2 = math.pi * (i + 1) / lat_div
        for j in range(lon_div):
            phi1 = 2 * math.pi * j / lon_div
            phi2 = 2 * math.pi * (j + 1) / lon_div

            # Four corner vertices for the current quad:
            v1 = (radius * math.sin(theta1) * math.cos(phi1),
                  radius * math.cos(theta1),
                  radius * math.sin(theta1) * math.sin(phi1))
            v2 = (radius * math.sin(theta1) * math.cos(phi2),
                  radius * math.cos(theta1),
                  radius * math.sin(theta1) * math.sin(phi2))
            v3 = (radius * math.sin(theta2) * math.cos(phi2),
                  radius * math.cos(theta2),
                  radius * math.sin(theta2) * math.sin(phi2))
            v4 = (radius * math.sin(theta2) * math.cos(phi1),
                  radius * math.cos(theta2),
                  radius * math.sin(theta2) * math.sin(phi1))

            # Texture coordinates based on spherical mapping:
            t1 = (phi1 / (2 * math.pi), theta1 / math.pi)
            t2 = (phi2 / (2 * math.pi), theta1 / math.pi)
            t3 = (phi2 / (2 * math.pi), theta2 / math.pi)
            t4 = (phi1 / (2 * math.pi), theta2 / math.pi)

            # Normals (for a unit sphere, same as the position normalized):
            n1 = (math.sin(theta1) * math.cos(phi1),
                  math.cos(theta1),
                  math.sin(theta1) * math.sin(phi1))
            n2 = (math.sin(theta1) * math.cos(phi2),
                  math.cos(theta1),
                  math.sin(theta1) * math.sin(phi2))
            n3 = (math.sin(theta2) * math.cos(phi2),
                  math.cos(theta2),
                  math.sin(theta2) * math.sin(phi2))
            n4 = (math.sin(theta2) * math.cos(phi1),
                  math.cos(theta2),
                  math.sin(theta2) * math.sin(phi1))

            # First triangle for this quad (v1, v2, v3)
            vertices.extend([v1, v2, v3])
            texcoords.extend([t1, t2, t3])
            normals.extend([n1, n2, n3])

            # Second triangle for this quad (v1, v3, v4)
            vertices.extend([v1, v3, v4])
            texcoords.extend([t1, t3, t4])
            normals.extend([n1, n3, n4])
    return vertices, texcoords, normals

def generate_ring(div, inner_radius, outer_radius):
    """
    Generates the vertices, texture coordinates, and normals for a flat ring (annulus).
    The ring lies in the xz-plane (y=0) and is built as a series of quads (two triangles per segment).
    """
    vertices = []
    texcoords = []
    normals = []
    for i in range(div):
        angle1 = 2 * math.pi * i / div
        angle2 = 2 * math.pi * (i + 1) / div

        # Four vertices defining one segment of the ring:
        v1 = (inner_radius * math.cos(angle1), 0.0, inner_radius * math.sin(angle1))
        v2 = (outer_radius * math.cos(angle1), 0.0, outer_radius * math.sin(angle1))
        v3 = (outer_radius * math.cos(angle2), 0.0, outer_radius * math.sin(angle2))
        v4 = (inner_radius * math.cos(angle2), 0.0, inner_radius * math.sin(angle2))

        # Texture coordinates: mapping the inner edge to v=0 and outer edge to v=1.
        t1 = (angle1 / (2 * math.pi), 0.0)
        t2 = (angle1 / (2 * math.pi), 1.0)
        t3 = (angle2 / (2 * math.pi), 1.0)
        t4 = (angle2 / (2 * math.pi), 0.0)

        # The ring is flat so all normals point up (0, 1, 0).
        n = (0.0, 1.0, 0.0)

        # First triangle (v1, v2, v3)
        vertices.extend([v1, v2, v3])
        texcoords.extend([t1, t2, t3])
        normals.extend([n, n, n])

        # Second triangle (v1, v3, v4)
        vertices.extend([v1, v3, v4])
        texcoords.extend([t1, t3, t4])
        normals.extend([n, n, n])
    return vertices, texcoords, normals

def format_mesh(primitive, texture_index, vertices, texcoords, normals):
    """
    Formats a single mesh into the custom file format.
    - primitive: 0 for GL_TRIANGLES (or 1/2 for strips/fans)
    - texture_index: an integer to select the texture (0 for sphere, 1 for ring, for example)
    """
    lines = []
    lines.append(f"tri {primitive} {texture_index}")
    lines.append(f"vert {len(vertices)}")
    for v in vertices:
        # Format each vertex coordinate (x y z)
        lines.append(f"{v[0]} {v[1]} {v[2]}")
    lines.append(f"tex {len(texcoords)}")
    for t in texcoords:
        # Format each texture coordinate (u v)
        lines.append(f"{t[0]} {t[1]}")
    lines.append(f"norm {len(normals)}")
    for n in normals:
        # Format each normal vector (x y z)
        lines.append(f"{n[0]} {n[1]} {n[2]}")
    return "\n".join(lines)

def main():
    # --- Generate Saturn's sphere (body) ---
    # Increase these numbers for a higher resolution sphere.
    sphere_lat_div = 30  
    sphere_lon_div = 60  
    sphere_vertices, sphere_texcoords, sphere_normals = generate_sphere(sphere_lat_div, sphere_lon_div)
    # Use primitive type 0 (GL_TRIANGLES) and texture index 0 for the sphere.
    sphere_mesh = format_mesh(0, 0, sphere_vertices, sphere_texcoords, sphere_normals)

    # --- Generate Saturn's ring ---
    # The ring is an annulus with inner and outer radii.
    ring_div = 40  
    inner_radius = 1.1  # slightly larger than the sphere radius
    outer_radius = 2.0  
    ring_vertices, ring_texcoords, ring_normals = generate_ring(ring_div, inner_radius, outer_radius)
    # Use primitive type 0 (GL_TRIANGLES) and texture index 1 for the ring.
    ring_mesh = format_mesh(0, 1, ring_vertices, ring_texcoords, ring_normals)

    # --- Write both meshes to a file ---
    with open("saturn_model.mxmod", "w") as f:
        f.write(sphere_mesh)
        f.write("\n")  # Separate the two meshes with a newline
        f.write(ring_mesh)
    print("Saturn model generated as 'saturn_model.mxmod'.")

if __name__ == "__main__":
    main()
