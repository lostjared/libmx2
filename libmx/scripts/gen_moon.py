
import sys
import math

def generate_crescent_mxmod(filename="moon.mxmod", 
                            arc_radius=10.0,    # Radius of the main curve
                            thickness=3.5,      # Thickness at the center
                            arc_degrees=260.0,  # Span of the moon (e.g. 260 degrees)
                            segments_arc=48, 
                            segments_cross=24,
                            normals_inward=False):
    """
    Generates a stylized Crescent Moon (tapered torus segment).
    - Default: Object mode (normals outward).
    - Set normals_inward=True for a skybox/tunnel.
    """
    
    vertices = []
    tex_coords = []
    normals = []

    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        if normals_inward:
            # Flip winding and normals for inside view
            vertices.extend([p1, p3, p2])
            tex_coords.extend([t1, t3, t2])
            normals.extend([(-n1[0], -n1[1], -n1[2]), 
                            (-n3[0], -n3[1], -n3[2]), 
                            (-n2[0], -n2[1], -n2[2])])
        else:
            vertices.extend([p1, p2, p3])
            tex_coords.extend([t1, t2, t3])
            normals.extend([n1, n2, n3])

    # Convert arc span to radians
    total_angle = math.radians(arc_degrees)
    start_angle = -total_angle / 2.0
    
    # Store rings of vertices to stitch later
    rings = []
    
    for i in range(segments_arc + 1):
        u = i / segments_arc
        
        # Current angle on the main arc (XY plane)
        theta = start_angle + (u * total_angle)
        
        # Spine position (Center of the moon's cross-section)
        sx = arc_radius * math.cos(theta)
        sy = arc_radius * math.sin(theta)
        sz = 0.0
        
        # Tapering radius: 0 at ends, 'thickness' at center
        # sin(0)=0, sin(pi)=0, sin(pi/2)=1
        r = thickness * math.sin(u * math.pi)
        
        # Basis vectors for the cross-section ring
        # B1: Radial vector outward from arc center (cos, sin, 0)
        # B2: Z axis (0, 0, 1)
        b1x, b1y, b1z = math.cos(theta), math.sin(theta), 0.0
        b2x, b2y, b2z = 0.0, 0.0, 1.0
        
        current_ring_verts = []
        
        for j in range(segments_cross + 1):
            v = j / segments_cross
            phi = v * 2.0 * math.pi
            
            # Circle in the B1/B2 plane
            c_cos = math.cos(phi)
            c_sin = math.sin(phi)
            
            # Offset from spine
            ox = r * (c_cos * b1x + c_sin * b2x)
            oy = r * (c_cos * b1y + c_sin * b2y)
            oz = r * (c_cos * b1z + c_sin * b2z)
            
            px = sx + ox
            py = sy + oy
            pz = sz + oz
            
            # Normal calculation
            nl = math.sqrt(ox*ox + oy*oy + oz*oz)
            if nl > 0:
                nx, ny, nz = ox/nl, oy/nl, oz/nl
            else:
                # At tips where r=0, use the radial vector
                nx, ny, nz = b1x, b1y, b1z
            
            current_ring_verts.append( ((px,py,pz), (u,v), (nx,ny,nz)) )
            
        rings.append(current_ring_verts)

    # Stitch rings together
    for i in range(segments_arc):
        for j in range(segments_cross):
            pA, tA, nA = rings[i][j]
            pB, tB, nB = rings[i][j+1]
            pC, tC, nC = rings[i+1][j+1]
            pD, tD, nD = rings[i+1][j]
            
            # Add Quad (2 Triangles)
            add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
            add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)

    # Write File
    num_verts = len(vertices)
    try:
        with open(filename, 'w') as f:
            f.write("tri 0 0\n")
            f.write(f"vert {num_verts}\n")
            for v in vertices:
                f.write(f"{v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            f.write("\n")
            f.write(f"tex {num_verts}\n")
            for t in tex_coords:
                f.write(f"{t[0]:.6f} {t[1]:.6f}\n")
            f.write("\n")
            f.write(f"norm {num_verts}\n")
            for n in normals:
                f.write(f"{n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
        print(f"Successfully generated {filename} with {num_verts} vertices.")
    except IOError as e:
        print(f"Error writing file: {e}")

if __name__ == "__main__":
    # Adjust parameters here
    generate_crescent_mxmod("moon.mxmod")