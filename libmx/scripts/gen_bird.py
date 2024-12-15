def write_section(file, vertices, texcoords, normals):
    """
    Writes a tri/vert/tex/norm section to the file.
    Assumes len(vertices) = len(texcoords) = len(normals).
    """
    file.write("tri 0 0\n")
    file.write(f"vert {len(vertices)}\n")
    for v in vertices:
        file.write(f"{v[0]} {v[1]} {v[2]}\n")

    file.write(f"tex {len(texcoords)}\n")
    for t in texcoords:
        file.write(f"{t[0]} {t[1]}\n")

    file.write(f"norm {len(normals)}\n")
    for n in normals:
        file.write(f"{n[0]} {n[1]} {n[2]}\n")

def main():
    # This script creates a rough SR-71 Blackbird-like model
    # Coordinates are not to scale and are a rough approximation.
    # The model will be composed of several sections:
    # 1. Main fuselage (simplified as a series of triangular strips)
    # 2. Wings
    # 3. Engine nacelles (intakes on sides)
    # 4. Tail fins
    
    # For simplicity, we will define some constants:
    # Length of fuselage: ~ 30 units
    # Wingspan: ~ 20 units total (10 units each side)
    # Height of fuselage: ~ 2 units
    # We will create a few sections and write them out.

    with open("blackbird_model.txt", "w") as f:
        # ------------------------------------------------------------
        # Main Fuselage
        # Imagine the fuselage as a series of triangles along the length.
        # We'll define a center spine and then upper and lower sections.
        
        # A simple approach: define quads as pairs of triangles.
        # We'll have a front tip, mid sections, and rear.
        
        # Let's define some key points along the fuselage centerline:
        # We'll create 5 segments along the fuselage from nose to tail.
        
        fuselage_points = [
            (0.0, 0.0, 15.0),   # Nose tip
            (0.0, 0.5, 7.5),    # Slightly behind nose, slightly up
            (0.0, 1.0, 0.0),    # Mid fuselage highest
            (0.0, 0.5, -7.5),   # Begin tapering down
            (0.0, 0.0, -15.0)   # Tail end
        ]
        
        # We'll create a simple cross-section for each segment:
        # The fuselage cross-section is like an oval.
        # For simplicity, define a few width points:
        
        def fuselage_cross_section(y, width=1.0, height=0.5):
            # returns list of points forming a diamond shape around centerline
            return [
                (-width, -height + y, 0.0),
                (width, -height + y, 0.0),
                (width, height + y, 0.0),
                (-width, height + y, 0.0)
            ]
        
        # We'll define widths at each fuselage point:
        widths = [0.2, 0.8, 1.0, 0.8, 0.3]
        heights = [0.1, 0.4, 0.5, 0.4, 0.1]
        
        # Generate fuselage geometry:
        # We'll "loft" from one section to the next.
        
        fuselage_vertices = []
        fuselage_tex = []
        fuselage_norm = []
        
        # Create triangles by connecting quad strips along fuselage
        # Each segment: we have top and bottom loops. We'll just do a simple set of triangles connecting them.
        
        # We'll assign texture coordinates based on length (z) and simple wrap around width:
        # We'll assign normals roughly along outward direction (approximate)
        
        # Build segments:
        for i in range(len(fuselage_points)-1):
            z1 = fuselage_points[i][2]
            y1 = fuselage_points[i][1]
            z2 = fuselage_points[i+1][2]
            y2 = fuselage_points[i+1][1]
            
            cs1 = fuselage_cross_section(y1, widths[i], heights[i])
            cs2 = fuselage_cross_section(y2, widths[i+1], heights[i+1])
            
            # cs1 and cs2 each have 4 points forming a loop
            # We'll form triangles between these 4-point loops:
            # (0->1->5, 0->5->4) etc, where loop is cs1:0..3 and cs2:0..3
            
            # Indices in cs1: 0..3, cs2:0..3
            # Triangles (two per quad):
            # (cs1[0], cs1[1], cs2[1]), (cs1[0], cs2[1], cs2[0])
            # (cs1[1], cs1[2], cs2[2]), (cs1[1], cs2[2], cs2[1])
            # (cs1[2], cs1[3], cs2[3]), (cs1[2], cs2[3], cs2[2])
            # (cs1[3], cs1[0], cs2[0]), (cs1[3], cs2[0], cs2[3])
            
            # We'll transform these local coords by adding the segment's z-offset (already considered)
            # Actually, we need to add z to the z-coordinates of cs points.
            
            def add_z(cs, zoffset):
                return [(x, y, z+zoffset) for (x,y,z) in cs]
            
            seg1 = add_z(cs1, z1)
            seg2 = add_z(cs2, z2)
            
            quads = [
                (0,1), (1,2), (2,3), (3,0)
            ]
            
            for (a,b) in quads:
                # two triangles per quad:
                # tri1: seg1[a], seg1[b], seg2[b]
                # tri2: seg1[a], seg2[b], seg2[a]
                
                # Compute some rough normals:
                # We'll just pick a normal by cross product approach (very rough).
                # For simplicity, just use a rough normal pointing outward based on vertex position.
                def calc_normal(p1, p2, p3):
                    import math
                    from math import sqrt
                    # p1,p2,p3 are tuples
                    u = (p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2])
                    v = (p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2])
                    nx = u[1]*v[2] - u[2]*v[1]
                    ny = u[2]*v[0] - u[0]*v[2]
                    nz = u[0]*v[1] - u[1]*v[0]
                    length = sqrt(nx*nx+ny*ny+nz*nz)
                    if length < 1e-9:
                        return (0.0,1.0,0.0)
                    return (nx/length, ny/length, nz/length)
                
                # texture coords: map x to u, z to v roughly
                # Just normalize and assign something simple:
                def texcoord(p):
                    # Just a simple mapping:
                    u = (p[0] + 10.0)/20.0
                    v = (p[2] + 15.0)/30.0
                    return (u,v)
                
                tri1 = (seg1[a], seg1[b], seg2[b])
                tri2 = (seg1[a], seg2[b], seg2[a])
                
                n1 = calc_normal(*tri1)
                n2 = calc_normal(*tri2)
                
                for p in tri1:
                    fuselage_vertices.append(p)
                    fuselage_tex.append(texcoord(p))
                    fuselage_norm.append(n1)
                
                for p in tri2:
                    fuselage_vertices.append(p)
                    fuselage_tex.append(texcoord(p))
                    fuselage_norm.append(n2)
        
        write_section(f, fuselage_vertices, fuselage_tex, fuselage_norm)
        
        # ------------------------------------------------------------
        # Wings
        # Let's create a simple pair of delta wings using two large triangular surfaces on each side.
        
        # Wing coordinates (approximate):
        # We'll place the wing root near the middle of the fuselage at z=0 and extend outwards.
        wing_vertices = []
        wing_tex = []
        wing_norm = []
        
        # Left wing (two triangles forming a simple shape):
        # Points (approx):
        # Root leading edge: (-0.5, 0.0, 5.0)
        # Tip leading edge: (-10.0, 0.0, 0.0)
        # Root trailing edge: (-0.5,0.0,-2.0)
        # Tip trailing edge: (-8.0, 0.0, -2.0)
        
        lw_pts = [
            (-0.5, 0.0, 5.0),   # A: root leading
            (-10.0, 0.0, 0.0),  # B: tip leading
            (-0.5, 0.0, -2.0),  # C: root trailing
            (-8.0, 0.0, -2.0)   # D: tip trailing
        ]
        
        # Triangles: (A,B,C) and (B,D,C)
        l_tris = [(lw_pts[0], lw_pts[1], lw_pts[2]),
                  (lw_pts[1], lw_pts[3], lw_pts[2])]
        
        # Similarly for right wing:
        rw_pts = [
            (0.5, 0.0, 5.0),    # A': root leading
            (10.0, 0.0, 0.0),   # B': tip leading
            (0.5, 0.0, -2.0),   # C': root trailing
            (8.0, 0.0, -2.0)    # D': tip trailing
        ]
        
        r_tris = [(rw_pts[0], rw_pts[1], rw_pts[2]),
                  (rw_pts[1], rw_pts[3], rw_pts[2])]
        
        def calc_normal_simple(p1, p2, p3):
            from math import sqrt
            u = (p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2])
            v = (p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2])
            nx = u[1]*v[2] - u[2]*v[1]
            ny = u[2]*v[0] - u[0]*v[2]
            nz = u[0]*v[1] - u[1]*v[0]
            length = sqrt(nx*nx+ny*ny+nz*nz)
            if length < 1e-9:
                return (0.0,1.0,0.0)
            return (nx/length, ny/length, nz/length)
        
        def simple_tex(p):
            u = (p[0]+10)/20.0
            v = (p[2]+15)/30.0
            return (u,v)
        
        # Add left wing
        for tri in l_tris:
            n = calc_normal_simple(*tri)
            for p in tri:
                wing_vertices.append(p)
                wing_tex.append(simple_tex(p))
                wing_norm.append(n)
        
        # Add right wing
        for tri in r_tris:
            n = calc_normal_simple(*tri)
            for p in tri:
                wing_vertices.append(p)
                wing_tex.append(simple_tex(p))
                wing_norm.append(n)
        
        write_section(f, wing_vertices, wing_tex, wing_norm)
        
        # ------------------------------------------------------------
        # Tail fins
        # Vertical tail:
        # A simple vertical tail on top at the rear:
        
        tail_vertices = []
        tail_tex = []
        tail_norm = []
        
        # Vertical tail triangle (just one fin):
        # Base at fuselage: (0.0,0.5,-14.0), (0.0,0.0,-14.0)
        # Tip: (0.0,3.0,-13.0)
        
        vfin = [
            (0.0, 0.5, -14.0),
            (0.0, 0.0, -14.0),
            (0.0, 3.0, -13.0)
        ]
        
        n = calc_normal_simple(*vfin)
        for p in vfin:
            tail_vertices.append(p)
            tail_tex.append(simple_tex(p))
            tail_norm.append(n)
        
        # Maybe a small ventral fin below:
        vfin2 = [
            (0.0, -0.1, -14.0),
            (0.0, 0.0, -14.0),
            (0.0, -0.5, -13.5)
        ]
        n2 = calc_normal_simple(*vfin2)
        for p in vfin2:
            tail_vertices.append(p)
            tail_tex.append(simple_tex(p))
            tail_norm.append(n2)
        
        write_section(f, tail_vertices, tail_tex, tail_norm)
        
        # ------------------------------------------------------------
        # Engine nacelles (just two simple cylinders on sides)
        # We'll approximate the engine nacelles as a pair of triangles each, very rough.
        
        eng_vertices = []
        eng_tex = []
        eng_norm = []
        
        # Left engine intake (just a triangle):
        # (near front): (-2.0, 0.0, 5.0), (-3.0,0.0,5.0), (-2.5,0.5,4.5)
        left_eng = [
            (-2.0, 0.0, 5.0),
            (-3.0, 0.0, 5.0),
            (-2.5, 0.5, 4.5)
        ]
        n3 = calc_normal_simple(*left_eng)
        for p in left_eng:
            eng_vertices.append(p)
            eng_tex.append(simple_tex(p))
            eng_norm.append(n3)
        
        # Right engine intake:
        right_eng = [
            (2.0, 0.0, 5.0),
            (3.0, 0.0, 5.0),
            (2.5, 0.5, 4.5)
        ]
        n4 = calc_normal_simple(*right_eng)
        for p in right_eng:
            eng_vertices.append(p)
            eng_tex.append(simple_tex(p))
            eng_norm.append(n4)
        
        write_section(f, eng_vertices, eng_tex, eng_norm)

if __name__ == "__main__":
    main()
