import math

def generate_sphere(radius, sectors, stacks, filename="sphere.mxmod"):
    vertices = []
    uvs = []
    normals = []
    
    # Generate positions, normals, and UVs
    for i in range(stacks + 1):
        phi = math.pi / 2 - i * math.pi / stacks
        y = radius * math.sin(phi)
        r_cos_phi = radius * math.cos(phi)
        
        # We loop to 'sectors + 1' to create the duplicate vertex for the seam
        for j in range(sectors + 1):
            theta = j * 2 * math.pi / sectors
            x = r_cos_phi * math.cos(theta)
            z = r_cos_phi * math.sin(theta)
            
            vertices.append((x, y, z))
            
            # UV mapping: s goes from 0 to 1, t goes from 0 to 1
            s = j / sectors
            t = i / stacks
            uvs.append((s, t))
            
            # Normal for a sphere is just the normalized position
            length = math.sqrt(x*x + y*y + z*z)
            normals.append((x/length, y/length, z/length))

    # Generate triangle indices
    indices = []
    for i in range(stacks):
        k1 = i * (sectors + 1)
        k2 = k1 + sectors + 1
        
        for j in range(sectors):
            if i != 0:
                indices.append((k1 + j, k2 + j, k1 + j + 1))
            if i != (stacks - 1):
                indices.append((k1 + j + 1, k2 + j, k2 + j + 1))

    # Write to mxmod format
    with open(filename, "w") as f:
        # Header: tri shape_type texture_index
        # shape_type 0 = GL_TRIANGLES
        f.write(f"tri 0 0\n")
        
        # Vertices
        f.write(f"vert {len(indices) * 3}\n")
        for tri in indices:
            for idx in tri:
                v = vertices[idx]
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        
        # Texture Coordinates
        f.write(f"tex {len(indices) * 3}\n")
        for tri in indices:
            for idx in tri:
                u = uvs[idx]
                f.write(f"{u[0]:.6f} {u[1]:.6f}\n")
                
        # Normals
        f.write(f"norm {len(indices) * 3}\n")
        for tri in indices:
            for idx in tri:
                n = normals[idx]
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")

if __name__ == "__main__":
    # Standard quality sphere
    generate_sphere(radius=1.0, sectors=32, stacks=16, filename="sphere.mxmod")
    print("mx: Generated sphere.mxmod with corrected UV seam.")
