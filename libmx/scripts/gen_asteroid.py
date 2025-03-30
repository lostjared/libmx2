#!/usr/bin/env python3
import math
import random

# Parameters for the asteroid
base_radius = 1.0         # Base sphere radius
noise_amplitude = 0.2     # Random noise added to the radius for irregularity
lat_div = 20              # Number of latitude subdivisions
lon_div = 40              # Number of longitude subdivisions

# Parameters for craters
num_craters = 5           # How many craters to add
crater_ang_radius = math.radians(10)  # Crater angular radius in radians (~10°)
crater_depth = 0.3        # Maximum depth of a crater

# Set random seed for reproducibility (optional)
random.seed(42)

# Generate random crater centers on the sphere (in spherical coordinates: theta, phi)
craters = []
for _ in range(num_craters):
    theta = random.uniform(0, math.pi)      # Inclination from 0 to pi
    phi = random.uniform(0, 2 * math.pi)      # Azimuth from 0 to 2pi
    craters.append((theta, phi))

def get_radius(theta, phi):
    """
    Calculate the vertex radius with noise and apply crater indentation.
    """
    # Base noise displacement
    r = base_radius + random.uniform(-noise_amplitude, noise_amplitude)
    
    # Apply each crater's effect (if the vertex is close to a crater center, subtract depth)
    for c_theta, c_phi in craters:
        # Compute the angular distance using the spherical law of cosines:
        cos_d = math.sin(theta)*math.sin(c_theta) + math.cos(theta)*math.cos(c_theta)*math.cos(phi - c_phi)
        # Clamp cos_d to avoid precision errors
        cos_d = max(min(cos_d, 1), -1)
        d = math.acos(cos_d)
        if d < crater_ang_radius:
            # Smooth falloff: full crater_depth at center, tapering off to zero at crater_ang_radius.
            # (Using a cosine falloff)
            factor = (1 + math.cos(math.pi * d / crater_ang_radius)) / 2
            r -= crater_depth * factor
    return r

# Create a grid for the sphere mesh (storing positions, texture coordinates, and normals)
grid = [[None for _ in range(lon_div + 1)] for _ in range(lat_div + 1)]
grid_tex = [[None for _ in range(lon_div + 1)] for _ in range(lat_div + 1)]
grid_normal = [[None for _ in range(lon_div + 1)] for _ in range(lat_div + 1)]

for i in range(lat_div + 1):
    theta = math.pi * i / lat_div  # theta varies from 0 to pi
    for j in range(lon_div + 1):
        phi = 2 * math.pi * j / lon_div  # phi varies from 0 to 2pi
        r = get_radius(theta, phi)
        # Convert spherical to Cartesian coordinates.
        x = r * math.sin(theta) * math.cos(phi)
        y = r * math.cos(theta)   # using y as "up"
        z = r * math.sin(theta) * math.sin(phi)
        grid[i][j] = (x, y, z)
        # Texture coordinates using spherical mapping
        u = phi / (2 * math.pi)
        v = theta / math.pi
        grid_tex[i][j] = (u, v)
        # Approximate normal: for a displaced sphere, using the normalized position vector.
        length = math.sqrt(x*x + y*y + z*z)
        grid_normal[i][j] = (x/length, y/length, z/length)

# Build triangle list: for each quad in the grid, create two triangles
triangles = []
for i in range(lat_div):
    for j in range(lon_div):
        # Four corners of the quad
        v0 = grid[i][j]
        v1 = grid[i+1][j]
        v2 = grid[i+1][j+1]
        v3 = grid[i][j+1]
        
        t0 = grid_tex[i][j]
        t1 = grid_tex[i+1][j]
        t2 = grid_tex[i+1][j+1]
        t3 = grid_tex[i][j+1]
        
        n0 = grid_normal[i][j]
        n1 = grid_normal[i+1][j]
        n2 = grid_normal[i+1][j+1]
        n3 = grid_normal[i][j+1]
        
        # Triangle 1: v0, v1, v2
        triangles.append(((v0, t0, n0), (v1, t1, n1), (v2, t2, n2)))
        # Triangle 2: v0, v2, v3
        triangles.append(((v0, t0, n0), (v2, t2, n2), (v3, t3, n3)))

# Flatten the triangle data into lists for vertices, texture coordinates, and normals.
all_vertices = []
all_tex = []
all_norm = []
for tri in triangles:
    for vertex in tri:
        pos, tex, norm = vertex
        all_vertices.append(pos)
        all_tex.append(tex)
        all_norm.append(norm)

# Write the model out in MXMOD format
with open("asteroid.mxmod", "w") as f:
    # 'tri 0 0' indicates triangle list (GL_TRIANGLES) with texture index 0.
    f.write("tri 0 0\n")
    # Write vertices
    f.write("vert {}\n".format(len(all_vertices)))
    for v in all_vertices:
        f.write("{:.4f} {:.4f} {:.4f}\n".format(v[0], v[1], v[2]))
    # Write texture coordinates
    f.write("tex {}\n".format(len(all_tex)))
    for t in all_tex:
        f.write("{:.4f} {:.4f}\n".format(t[0], t[1]))
    # Write normals
    f.write("norm {}\n".format(len(all_norm)))
    for n in all_norm:
        f.write("{:.4f} {:.4f} {:.4f}\n".format(n[0], n[1], n[2]))

print("Asteroid model with craters generated and saved to 'asteroid.mxmod'.")
