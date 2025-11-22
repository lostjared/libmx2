import math
import sys

def generate_octopus_mxmod(filename="octopus.mxmod", 
                           num_arms=8, 
                           arm_length=80.0, 
                           hub_radius=15.0, 
                           arm_radius_start=6.0,
                           arm_radius_end=1.0,
                           segments_len=32, 
                           segments_rad=16,
                           reverse_winding=False):
    """
    Generates an Octopus-shaped skybox (Hub + 8 Wavy Arms).
    - Normals point INWARD.
    - reverse_winding: Set to True if you see a black screen (depends on your glFrontFace setting).
    """
    
    vertices = []
    tex_coords = []
    normals = []

    # --- Helper: Add Triangle ---
    def add_triangle(p1, p2, p3, t1, t2, t3, n1, n2, n3):
        # If reverse_winding is True, swap p2/p3 to flip visibility
        if reverse_winding:
            vertices.extend([p1, p3, p2])
            tex_coords.extend([t1, t3, t2])
            normals.extend([n1, n3, n2])
        else:
            vertices.extend([p1, p2, p3])
            tex_coords.extend([t1, t2, t3])
            normals.extend([n1, n2, n3])

    # --- 1. Generate Central Hub (Sphere) ---
    # We generate a sphere but leave holes for arms? 
    # For simplicity, we generate a full sphere. The arms will intersect it.
    
    hub_stacks = 16
    hub_slices = 32
    
    for i in range(hub_stacks):
        for j in range(hub_slices):
            # Sphere math
            def get_sphere_vert(row, col):
                u = col / hub_slices
                v = row / hub_stacks
                phi = v * math.pi
                theta = u * 2.0 * math.pi
                
                x = hub_radius * math.sin(phi) * math.cos(theta)
                y = hub_radius * math.cos(phi)
                z = hub_radius * math.sin(phi) * math.sin(theta)
                
                # Normal points inward (-x, -y, -z)
                l = math.sqrt(x*x + y*y + z*z)
                nx, ny, nz = -x/l, -y/l, -z/l
                
                return (x,y,z), (u,v), (nx,ny,nz)

            pA, tA, nA = get_sphere_vert(i, j)
            pB, tB, nB = get_sphere_vert(i+1, j)
            pC, tC, nC = get_sphere_vert(i+1, j+1)
            pD, tD, nD = get_sphere_vert(i, j+1)
            
            # Add Quad (Inside View -> CCW)
            add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
            add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)

    # --- 2. Generate Arms ---
    for arm_i in range(num_arms):
        # Base angle for this arm
        arm_angle_base = (arm_i / num_arms) * 2.0 * math.pi
        
        # Direction vector for this arm in XZ plane
        dir_x = math.cos(arm_angle_base)
        dir_z = math.sin(arm_angle_base)
        
        # Generate segments along the arm
        for i in range(segments_len):
            # Progress (0.0 to 1.0)
            prog_curr = i / segments_len
            prog_next = (i + 1) / segments_len
            
            # Calculate Center Points of the tube at Current and Next steps
            # We add a sine wave to Y for "waviness"
            def get_center_pos(t):
                dist = hub_radius * 0.8 + (t * arm_length) # Start slightly inside hub
                
                # Wavy motion
                wave = math.sin(t * math.pi * 2.0) * (arm_length * 0.2)
                
                cx = dir_x * dist
                cz = dir_z * dist
                cy = wave
                return cx, cy, cz

            cx1, cy1, cz1 = get_center_pos(prog_curr)
            cx2, cy2, cz2 = get_center_pos(prog_next)
            
            # Calculate Radius at these points (Tapering)
            r1 = arm_radius_start * (1.0 - prog_curr) + arm_radius_end * prog_curr
            r2 = arm_radius_start * (1.0 - prog_next) + arm_radius_end * prog_next
            
            # Calculate a basis frame for the ring (Forward, Up, Right)
            # Forward is vector from c1 to c2
            fx, fy, fz = cx2-cx1, cy2-cy1, cz2-cz1
            fl = math.sqrt(fx*fx + fy*fy + fz*fz)
            fx, fy, fz = fx/fl, fy/fl, fz/fl
            
            # Up vector (approximate global up, then orthogonalize)
            ux, uy, uz = 0.0, 1.0, 0.0
            # Right = Up x Forward
            rx = uy*fz - uz*fy
            ry = uz*fx - ux*fz
            rz = ux*fy - uy*fx
            rl = math.sqrt(rx*rx + ry*ry + rz*rz)
            if rl < 0.001: # Handle vertical case
                rx, ry, rz = 1.0, 0.0, 0.0
            else:
                rx, ry, rz = rx/rl, ry/rl, rz/rl
            
            # Re-calc Up = Forward x Right
            ux = fy*rz - fz*ry
            uy = fz*rx - fx*rz
            uz = fx*ry - fy*rx
            
            # Generate Ring Vertices
            def get_ring_verts(cx, cy, cz, r, u_offset):
                ring_verts = []
                for j in range(segments_rad + 1): # +1 to wrap UVs
                    theta = (j / segments_rad) * 2.0 * math.pi
                    
                    # Circle in local frame
                    cost = math.cos(theta)
                    sint = math.sin(theta)
                    
                    # Pos = Center + r * (cos * Right + sin * Up)
                    px = cx + r * (cost * rx + sint * ux)
                    py = cy + r * (cost * ry + sint * uy)
                    pz = cz + r * (cost * rz + sint * uz)
                    
                    # Normal (Inward) points to center line
                    # Vector from Pos to Center
                    nx = cx - px
                    ny = cy - py
                    nz = cz - pz
                    nl = math.sqrt(nx*nx + ny*ny + nz*nz)
                    if nl > 0: nx, ny, nz = nx/nl, ny/nl, nz/nl
                    
                    uv = (j / segments_rad, u_offset)
                    
                    ring_verts.append( ((px,py,pz), uv, (nx,ny,nz)) )
                return ring_verts

            ring1 = get_ring_verts(cx1, cy1, cz1, r1, prog_curr)
            ring2 = get_ring_verts(cx2, cy2, cz2, r2, prog_next)
            
            # Stitch rings
            for j in range(segments_rad):
                pA, tA, nA = ring1[j]
                pB, tB, nB = ring1[j+1]
                pC, tC, nC = ring2[j+1]
                pD, tD, nD = ring2[j]
                
                add_triangle(pA, pB, pD, tA, tB, tD, nA, nB, nD)
                add_triangle(pB, pC, pD, tB, tC, tD, nB, nC, nD)

    # --- Write File ---
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
    # NOTE: If you still see a black screen, change reverse_winding to True
    generate_octopus_mxmod("octopus.mxmod", reverse_winding=False)