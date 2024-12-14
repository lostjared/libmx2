import math

def generate_sphere(M=12, N=6):
    """
    Generate a sphere made of triangles.
    M: number of longitudinal divisions
    N: number of latitudinal divisions
    A total of N+1 'rings' of vertices, from 0 (top) to N (bottom).
    """

    # Generate vertices
    # Latitude lines: i=0..N (N+1 total)
    # phi goes from 0 at top (y=1) to pi at bottom (y=-1)
    # Longitude lines: j=0..M-1
    # theta goes from 0 to 2pi

    vertices = []
    texcoords = []
    normals = []

    # Precompute vertices, normals and tex for the grid
    for i in range(N+1):
        phi = math.pi * i / N
        y = math.cos(phi)
        r = math.sin(phi)
        for j in range(M):
            theta = 2*math.pi * j / M
            x = r * math.cos(theta)
            z = r * math.sin(theta)

            # vertex
            vertices.append((x,y,z))
            # normal (same as vertex for unit sphere)
            normals.append((x,y,z))
            # texture coordinates
            u = j/float(M)
            v = 1.0 - (i/float(N))
            texcoords.append((u,v))

    # Now generate triangles
    # Each 'quad' formed by (i,j), (i+1,j), (i,j+1), (i+1,j+1) → two triangles
    # Top row (i=0) and bottom row (i=N-1) are special (triangle fans).
    # Index function: V(i,j) = i*M + j
    def idx(i,j):
        return i*M + (j % M)

    triangles = []
    # Top fan: i=0 uses the single top vertex at i=0 and band i=1
    # top vertex is at all longitudes the same point, but we stored M identical top points.
    # We will just pick one as the "top pole" and form triangles with it.
    top_pole_index = 0  # i=0, j=0 chosen as top pole
    for j in range(M):
        # tri: top_pole_index, V(1,j), V(1,j+1)
        triangles.append((top_pole_index, idx(1,j), idx(1,j+1)))

    # Middle bands
    for i in range(1, N-1):
        for j in range(M):
            # two triangles forming a quad:
            # (i,j), (i+1,j), (i,j+1)
            # (i+1,j), (i+1,j+1), (i,j+1)
            t1 = (idx(i,j),   idx(i+1,j), idx(i,j+1))
            t2 = (idx(i+1,j), idx(i+1,j+1), idx(i,j+1))
            triangles.append(t1)
            triangles.append(t2)

    # Bottom fan: i=N is bottom line
    bottom_pole_index = N*M  # i=N, j=0 chosen as bottom pole
    for j in range(M):
        # tri: bottom_pole_index, V(N-1,j+1), V(N-1,j)
        triangles.append((bottom_pole_index, idx(N-1,j+1), idx(N-1,j)))

    # Now we have:
    # - vertices, texcoords, normals arrays of length (N+1)*M + 2 (actually top and bottom share M identical points; 
    #   we used just the first for top pole and last for bottom pole. 
    # Wait, we must ensure top_pole_index and bottom_pole_index are correct:
    # Actually we stored M vertices for i=0 and M vertices for i=N. The top and bottom are repeated M times.
    # We picked top_pole_index=0 (that's fine).
    # For bottom pole, we do the same approach: choose bottom_pole_index=N*M (the start of the bottom ring)
    # But bottom ring also has M identical points. We'll just use idx(N,0) as bottom pole and reference that single index.

    # Correction: Let's actually just treat the top and bottom as they are:
    # We created N+1 rows. The top row (i=0) is all the same vertex repeated M times.
    # bottom row (i=N) is also all the same vertex repeated M times.
    # top_pole_index = idx(0,0)
    # bottom_pole_index = idx(N,0)
    # Update triangles for top and bottom fans using these consistent indices.

def print_sphere(M=12, N=6):
    # Regenerate inside print_sphere for clarity
    import math

    vertices = []
    normals = []
    texcoords = []

    for i in range(N+1):
        phi = math.pi * i / N
        y = math.cos(phi)
        r = math.sin(phi)
        for j in range(M):
            theta = 2*math.pi * j / M
            x = r * math.cos(theta)
            z = r * math.sin(theta)
            vertices.append((x,y,z))
            normals.append((x,y,z))
            u = j/float(M)
            v = 1.0 - i/float(N)
            texcoords.append((u,v))

    def idx(i,j):
        return i*M + (j % M)

    top_pole_index = idx(0,0)
    bottom_pole_index = idx(N,0)

    triangles = []
    # Top fan
    for j in range(M):
        # top: (top_pole, V(1,j), V(1,j+1))
        triangles.append((top_pole_index, idx(1,j), idx(1,j+1)))

    # Middle bands
    for i in range(1, N-1):
        for j in range(M):
            t1 = (idx(i,j), idx(i+1,j), idx(i,j+1))
            t2 = (idx(i+1,j), idx(i+1,j+1), idx(i,j+1))
            triangles.append(t1)
            triangles.append(t2)

    # Bottom fan
    for j in range(M):
        # bottom: (bottom_pole, V(N-1,j+1), V(N-1,j))
        triangles.append((bottom_pole_index, idx(N-1,j+1), idx(N-1,j)))

    # Print in the requested format
    T = len(triangles)
    print("tri 0 0\n")
    print(f"vert {T*3} 0")

    # For printing, we must print each triangle's vertices in order
    # Keep track of line numbers and also print comments

    # We'll store the per-triangle vertices, tex, norm in arrays to print after
    tri_verts = []
    tri_tex = []
    tri_norms = []

    for t_i, tri in enumerate(triangles, start=1):
        print(f"# Triangle {t_i}")
        for v_i in tri:
            vx, vy, vz = vertices[v_i]
            print(f"{vx} {vy} {vz}")
            tri_verts.append((vx,vy,vz))

    # Texture coordinates
    print(f"\ntex {T*3}")
    for t_i in range(T):
        print(f"# Tri {t_i+1}")
        base = t_i*3
        for k in range(3):
            u,v = texcoords[triangles[t_i][k]]
            print(f"{u} {v}")

    # Normals
    print(f"\nnorm {T*3}")
    for t_i in range(T):
        print(f"# Tri {t_i+1}")
        for k in range(3):
            nx, ny, nz = normals[triangles[t_i][k]]
            print(f"{nx} {ny} {nz}")


if __name__ == "__main__":
    # Adjust M and N for smoother spheres if desired.
    # M - number of segments around (longitudes)
    # N - number of segments from top to bottom (latitudes)
    M = 64
    N = 48
    print_sphere(M, N)
