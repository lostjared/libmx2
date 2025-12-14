import math

# --- SETTINGS ---
FILENAME = "sphere_pillar.mxmod"  # This will overwrite your first file
SPHERE_RADIUS = 10.0
PILLAR_RADIUS = 1.0

def generate_file():
    vertices = []
    uvs = []
    normals = []

    def add_quad(v1, v2, v3, v4):
        # Split quad into two triangles
        # Triangle 1
        vertices.extend([v1['pos'], v2['pos'], v3['pos']])
        uvs.extend([v1['uv'], v2['uv'], v3['uv']])
        normals.extend([v1['norm'], v2['norm'], v3['norm']])
        # Triangle 2
        vertices.extend([v1['pos'], v3['pos'], v4['pos']])
        uvs.extend([v1['uv'], v3['uv'], v4['uv']])
        normals.extend([v1['norm'], v3['norm'], v4['norm']])

    # 1. SPHERE (Inverted - Room)
    rings = 16
    segments = 32
    
    for r in range(rings):
        for s in range(segments):
            curr_ring = r
            next_ring = r + 1
            curr_seg = s
            next_seg = (s + 1) % segments

            def get_sphere_vert(r_idx, s_idx):
                phi = math.pi * float(r_idx) / rings
                theta = 2.0 * math.pi * float(s_idx) / segments
                
                # Geometry
                x = SPHERE_RADIUS * math.sin(phi) * math.cos(theta)
                y = SPHERE_RADIUS * math.cos(phi)
                z = SPHERE_RADIUS * math.sin(phi) * math.sin(theta)
                
                # UV
                u = float(s_idx) / segments
                v = float(r_idx) / rings
                
                # Normal (Inward)
                l = math.sqrt(x*x + y*y + z*z)
                if l == 0: l = 1
                nx, ny, nz = -x/l, -y/l, -z/l
                
                return {'pos': (x,y,z), 'uv': (u,v), 'norm': (nx,ny,nz)}

            p1 = get_sphere_vert(curr_ring, curr_seg)
            p2 = get_sphere_vert(next_ring, curr_seg)
            p3 = get_sphere_vert(next_ring, next_seg)
            p4 = get_sphere_vert(curr_ring, next_seg)

            # Inverted winding for inside view
            add_quad(p4, p3, p2, p1)

    # 2. PILLAR (Cylinder)
    # We set height from -SPHERE_RADIUS to +SPHERE_RADIUS
    # This ensures it touches the very top and bottom poles.
    y_top = SPHERE_RADIUS
    y_btm = -SPHERE_RADIUS
    
    pillar_segs = 32
    
    for s in range(pillar_segs):
        curr_s = s
        next_s = (s + 1) % pillar_segs
        
        theta1 = 2.0 * math.pi * float(curr_s) / pillar_segs
        theta2 = 2.0 * math.pi * float(next_s) / pillar_segs
        
        x1 = PILLAR_RADIUS * math.cos(theta1)
        z1 = PILLAR_RADIUS * math.sin(theta1)
        x2 = PILLAR_RADIUS * math.cos(theta2)
        z2 = PILLAR_RADIUS * math.sin(theta2)
        
        # Normals (Outward)
        n1 = (math.cos(theta1), 0, math.sin(theta1))
        n2 = (math.cos(theta2), 0, math.sin(theta2))
        
        v1 = {'pos': (x1, y_btm, z1), 'uv': (s/pillar_segs, 0), 'norm': n1}
        v2 = {'pos': (x1, y_top, z1), 'uv': (s/pillar_segs, 1), 'norm': n1}
        v3 = {'pos': (x2, y_top, z2), 'uv': ((s+1)/pillar_segs, 1), 'norm': n2}
        v4 = {'pos': (x2, y_btm, z2), 'uv': ((s+1)/pillar_segs, 0), 'norm': n2}
        
        add_quad(v1, v2, v3, v4)

    # WRITE FILE
    with open(FILENAME, 'w') as f:
        count = len(vertices)
        f.write("tri 0 0\n")
        
        f.write(f"vert {count}\n")
        for x,y,z in vertices:
            f.write(f"{x:.6f} {y:.6f} {z:.6f}\n")
        f.write("\n")
            
        f.write(f"tex {count}\n")
        for u,v in uvs:
            f.write(f"{u:.6f} {v:.6f}\n")
        f.write("\n")
            
        f.write(f"norm {count}\n")
        for nx,ny,nz in normals:
            f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")
            
    print(f"DONE! Overwrote {FILENAME}. Please reload it in your viewer.")

if __name__ == "__main__":
    generate_file()