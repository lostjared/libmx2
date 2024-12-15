import math

def normalize(v):
    length = math.sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])
    if length < 1e-9:
        return (0.0, 0.0, 1.0)
    return (v[0]/length, v[1]/length, v[2]/length)

def main():
    # Parameters for the saucer shape
    radius = 1.0    # horizontal radius
    height = 0.5    # half of the vertical dimension
    numRings = 16   # number of divisions from top to bottom
    numSegments = 32 # number of divisions around the circumference
    
    # Generate vertex grid
    # φ goes from 0 (top) to π (bottom)
    # θ goes from 0 to 2π
    vertices = []
    normals = []
    texcoords = []
    
    for i in range(numRings+1):
        phi = (math.pi * i) / numRings
        sin_phi = math.sin(phi)
        cos_phi = math.cos(phi)
        
        for j in range(numSegments+1):
            theta = (2.0 * math.pi * j) / numSegments
            cos_theta = math.cos(theta)
            sin_theta = math.sin(theta)
            
            # Compute vertex position
            x = radius * sin_phi * cos_theta
            y = radius * sin_phi * sin_theta
            z = height * cos_phi
            
            # Compute texture coords
            # u = θ/(2π), v = φ/π
            u = float(j)/numSegments
            v = float(i)/numRings
            
            # Compute normal:
            # For an ellipsoid-like shape, scale coordinates by (1/radius, 1/radius, 1/height) then normalize
            nx, ny, nz = normalize((x/radius, y/radius, z/height))
            
            vertices.append((x, y, z))
            texcoords.append((u, v))
            normals.append((nx, ny, nz))
    
    # Construct triangles
    # Each pair of rows forms a set of quads, each split into two triangles
    # For ring i and segment j:
    # upper-left = i*(numSegments+1) + j
    # lower-left = (i+1)*(numSegments+1) + j
    # upper-right = i*(numSegments+1) + j+1
    # lower-right = (i+1)*(numSegments+1) + j+1
    # Triangles: (UL, LL, LR) and (UL, LR, UR)
    
    indices = []
    for i in range(numRings):
        for j in range(numSegments):
            ul = i*(numSegments+1) + j
            ur = i*(numSegments+1) + j+1
            ll = (i+1)*(numSegments+1) + j
            lr = (i+1)*(numSegments+1) + j+1
            
            # First triangle
            indices.append(ul)
            indices.append(ll)
            indices.append(lr)
            
            # Second triangle
            indices.append(ul)
            indices.append(lr)
            indices.append(ur)
    
    # Now we have all vertices and indices
    # We will output in the given format:
    # tri 0 0
    # vert <count_of_verts_in_triangles>
    # ...
    # tex <count>
    # ...
    # norm <count>
    # ...
    #
    # Note that we must duplicate the vertex/tex/normal data according to the indices we created.
    
    final_vertices = []
    final_texcoords = []
    final_normals = []
    for idx in indices:
        final_vertices.append(vertices[idx])
        final_texcoords.append(texcoords[idx])
        final_normals.append(normals[idx])
    
    # Write to a file
    with open("flying_saucer.model", "w") as f:
        f.write("tri 0 0\n")
        f.write(f"vert {len(final_vertices)}\n")
        for vx, vy, vz in final_vertices:
            f.write(f"{vx:.6f} {vy:.6f} {vz:.6f}\n")
        
        f.write(f"tex {len(final_texcoords)}\n")
        for u,v in final_texcoords:
            f.write(f"{u:.6f} {v:.6f}\n")
        
        f.write(f"norm {len(final_normals)}\n")
        for nx, ny, nz in final_normals:
            f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")

if __name__ == "__main__":
    main()
