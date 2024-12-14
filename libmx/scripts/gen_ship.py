import math

def normalize(v):
    length = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if length > 1e-9:
        return (v[0]/length, v[1]/length, v[2]/length)
    return (0.0,0.0,0.0)

def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def gen_hull(M=24, N=6):
    """
    Generate a stretched ellipsoidal hull.
    We map i in [0..N] for latitude (phi), j in [0..M] for longitude (theta).
    Let's shape: 
    x = sin(phi)*cos(theta)*0.5 (narrow in x)
    y = cos(phi)*1.5 (elongated vertically)
    z = sin(phi)*sin(theta)*1.0 (longer in z)
    
    phi: 0 at top, pi at bottom
    """
    vertices = []
    texcoords = []
    for i in range(N+1):
        phi = math.pi * i / N
        y = 1.5 * math.cos(phi)
        r = math.sin(phi)
        for j in range(M):
            theta = 2*math.pi * j / M
            x = 0.5 * r * math.cos(theta)
            z = r * math.sin(theta)
            vertices.append((x,y,z))
            u = j/float(M)
            v = 1.0 - i/float(N)
            texcoords.append((u,v))
    # Triangles for the hull
    triangles = []
    # top fan
    top = 0
    for j in range(M):
        triangles.append((top, (1*M+j) % (M*(N+1)), (1*M+(j+1)%M) % (M*(N+1))))
    # middle quads
    for i in range(1,N-1):
        for j in range(M):
            v1 = i*M+j
            v2 = (i+1)*M+j
            v3 = i*M+(j+1)%M
            v4 = (i+1)*M+(j+1)%M
            triangles.append((v1,v2,v3))
            triangles.append((v2,v4,v3))
    # bottom fan
    bottom = N*M
    for j in range(M):
        triangles.append((bottom, ((N-1)*M+(j+1)%M), ((N-1)*M+j)))
    return vertices, texcoords, triangles

def gen_wings(base_vertices_count, M=4, N=2):
    """
    Generate simple flat wings on the sides of the hull.
    We'll attach wings around the mid-latitude of the hull.
    Assume hull center at i = N/2 for demonstration.
    We will just add a pair of trapezoidal wings extending in +x/-x direction.
    
    We'll pick a ring from the hull and extrude wings out.
    Let's just make two flat wings as separate geometry "patch".
    """
    # We'll define a very simple wing shape made of two quads (4 triangles).
    # Wing extends from y ~ 0 region, we can place them near the midpoint of hull.
    # We'll pick a certain coordinate for the wing root and tip.
    
    # For simplicity, let's just hardcode a small wing geometry:
    # A wing: 2 triangles forming a quad
    # Positions relative to hull center:
    # left wing (extending in -x), right wing (extending in +x)
    # We'll place them at y=0 for simplicity.
    
    # Wing coordinates:
    # Right wing quad:
    # root inner: (0.1,0.0,0.3)
    # root outer: (1.0,0.0,0.3)
    # tip inner: (0.1,0.0,-0.3)
    # tip outer: (1.0,0.0,-0.3)
    # Similarly left wing mirrored in x:
    # (-0.1,0.0,0.3), (-1.0,0.0,0.3), (-0.1,0.0,-0.3), (-1.0,0.0,-0.3)
    
    wing_verts = [
        # Right wing vertices:
        (0.1,0.0,0.3),
        (1.0,0.0,0.3),
        (0.1,0.0,-0.3),
        (1.0,0.0,-0.3),
        # Left wing vertices:
        (-0.1,0.0,0.3),
        (-1.0,0.0,0.3),
        (-0.1,0.0,-0.3),
        (-1.0,0.0,-0.3)
    ]
    
    # Texture coordinates for wings (just a simple mapping):
    # Let's do (u,v) from 0..1 across the wing surface
    wing_tex = [
        (0.0,1.0),(1.0,1.0),(0.0,0.0),(1.0,0.0), # right wing quad
        (0.0,1.0),(1.0,1.0),(0.0,0.0),(1.0,0.0)  # left wing quad
    ]
    
    # Triangles: two quads each split into two triangles
    # Right wing:
    # quad: (0,1,2,3)
    # tri1: (0,1,2), tri2: (1,3,2)
    # Left wing:
    # quad: (4,5,6,7)
    # tri3: (4,5,6), tri4: (5,7,6)
    
    triangles = [
        (base_vertices_count+0, base_vertices_count+1, base_vertices_count+2),
        (base_vertices_count+1, base_vertices_count+3, base_vertices_count+2),
        (base_vertices_count+4, base_vertices_count+5, base_vertices_count+6),
        (base_vertices_count+5, base_vertices_count+7, base_vertices_count+6)
    ]
    
    return wing_verts, wing_tex, triangles

def compute_triangle_normals(vertices, triangles):
    # Compute normals for each triangle vertex by using face normal.
    # We will assign the same normal for all three vertices of a triangle.
    # For a smoother model, you'd average normals, but here we'll just use face normals.
    normals = []
    for t in triangles:
        v0 = vertices[t[0]]
        v1 = vertices[t[1]]
        v2 = vertices[t[2]]
        e1 = (v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2])
        e2 = (v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2])
        n = normalize(cross(e1,e2))
        # same normal for all
        normals.append(n)
        normals.append(n)
        normals.append(n)
    return normals

def main():
    # Generate hull
    hull_vertices, hull_tex, hull_tri = gen_hull(M=24,N=6)
    
    # Add wings
    wing_vertices, wing_tex, wing_tri = gen_wings(len(hull_vertices))
    
    # Combine
    all_vertices = hull_vertices + wing_vertices
    all_tex = hull_tex + wing_tex
    all_tri = hull_tri + wing_tri
    
    # Compute normals
    # one normal per vertex in each triangle
    # We'll print after computing normals
    # But we need them per-triangle-vertex basis in order, so let's do it now:
    normals = compute_triangle_normals(all_vertices, all_tri)
    
    # Print in desired format
    print("tri 0 0\n")
    print(f"vert {len(all_tri)*3} 0")
    # For printing, we must go triangle by triangle, vertex by vertex:
    # Store ordered lists as we print
    ordered_vertices = []
    ordered_tex = []
    ordered_normals = []
    for i, t in enumerate(all_tri, start=1):
        print(f"# Triangle {i}")
        for vi in t:
            v = all_vertices[vi]
            print(f"{v[0]} {v[1]} {v[2]}")
            ordered_vertices.append(v)
    
    # Print texture coords
    print(f"\ntex {len(all_tri)*3}")
    for i, t in enumerate(all_tri, start=1):
        print(f"# Tri {i}")
        for vi in t:
            u,v = all_tex[vi]
            print(f"{u} {v}")
            ordered_tex.append((u,v))
    
    # Print normals
    print(f"\nnorm {len(all_tri)*3}")
    for i, t in enumerate(all_tri, start=1):
        print(f"# Tri {i}")
        # we already computed normals in order for each tri above
        base = (i-1)*3
        for k in range(3):
            nx, ny, nz = normals[base+k]
            print(f"{nx} {ny} {nz}")

if __name__ == "__main__":
    main()
