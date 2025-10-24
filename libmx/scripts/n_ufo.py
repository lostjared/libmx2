import math
import sys

SEG_THETA = 96
SEG_PROFILE = 48

profile = [
    (0.00,  0.50),
    (0.18,  0.47),
    (0.40,  0.41),
    (0.70,  0.28),
    (0.95,  0.12),
    (1.00,  0.00),
    (0.92, -0.04),
    (0.62, -0.10),
    (0.28, -0.15),
    (0.06, -0.18),
    (0.00, -0.19)
]

def lerp(a,b,t): return a+(b-a)*t

def sample_profile(points, steps):
    segs = len(points)-1
    out = []
    lengths = [0.0]
    acc = 0.0
    for i in range(segs):
        x0,y0 = points[i]
        x1,y1 = points[i+1]
        dx,dy = x1-x0,y1-y0
        seglen = math.hypot(dx,dy)
        acc += seglen
        lengths.append(acc)
    total = lengths[-1] if lengths[-1] > 0 else 1.0
    for i in range(steps):
        t = i/(steps-1)
        u = t*segs
        k = int(math.floor(u))
        if k >= segs: k = segs-1
        f = u - k
        x0,y0 = points[k]
        x1,y1 = points[k+1]
        x = lerp(x0,x1,f)
        y = lerp(y0,y1,f)
        if k == 0: xm,ym = points[0]; xp,yp = points[1]
        elif k == segs-1: xm,ym = points[segs-1]; xp,yp = points[segs]
        else: xm,ym = points[k]; xp,yp = points[k+1]
        dx = xp - xm
        dy = yp - ym
        seglen = math.hypot(dx,dy)
        if seglen == 0: dx,dy = 0.0,1.0
        else: dx,dy = dx/seglen, dy/seglen
        s_along = t
        out.append((x,y,dx,dy,s_along))
    return out

prof = sample_profile(profile, SEG_PROFILE)

verts = []
tex = []
norms = []

def add_tri(v0,t0,n0,v1,t1,n1,v2,t2,n2):
    verts.extend(v0); verts.extend(v1); verts.extend(v2)
    tex.extend(t0); tex.extend(t1); tex.extend(t2)
    norms.extend(n0); norms.extend(n1); norms.extend(n2)

for i in range(SEG_THETA):
    j = (i+1) % SEG_THETA
    th0 = (i/SEG_THETA)*2*math.pi
    th1 = (j/SEG_THETA)*2*math.pi
    c0,s0 = math.cos(th0), math.sin(th0)
    c1,s1 = math.cos(th1), math.sin(th1)
    for r in range(SEG_PROFILE-1):
        r0 = prof[r]
        r1 = prof[r+1]
        R0,Y0,DX0,DY0,V0 = r0
        R1,Y1,DX1,DY1,V1 = r1

        p00 = ( R0*c0, Y0, R0*s0 )
        p10 = ( R0*c1, Y0, R0*s1 )
        p01 = ( R1*c0, Y1, R1*s0 )
        p11 = ( R1*c1, Y1, R1*s1 )

        u0 = i/SEG_THETA
        u1 = j/SEG_THETA
        t00 = (u0, V0)
        t10 = (u1, V0)
        t01 = (u0, V1)
        t11 = (u1, V1)

        dth0 = (-R0*s0, 0.0, R0*c0)
        dth1 = (-R1*s1, 0.0, R1*c1)
        dpr0 = (DX0*c0, DY0, DX0*s0)
        dpr1 = (DX1*c1, DY1, DX1*s1)

        def cross(a,b):
            return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
        def norm(v):
            l=math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
            if l==0: return (0.0,1.0,0.0)
            return (v[0]/l, v[1]/l, v[2]/l)

        n00 = norm(cross(dth0, dpr0))
        n10 = norm(cross(dth0, (DX0*c1, DY0, DX0*s1)))
        n01 = norm(cross(dth1, dpr1))
        n11 = norm(cross(dth1, (DX1*c0, DY1, DX1*s0)))

        add_tri(p00,t00,n00, p10,t10,n10, p11,t11,n11)
        add_tri(p00,t00,n00, p11,t11,n11, p01,t01,n01)

print("tri 0 0")
print(f"vert {len(verts)//3}")
for i in range(0,len(verts),3):
    print(f"{verts[i]:.6f} {verts[i+1]:.6f} {verts[i+2]:.6f}")
print(f"\ntex {len(tex)//2}")
for i in range(0,len(tex),2):
    print(f"{tex[i]:.6f} {tex[i+1]:.6f}")
print(f"\nnorm {len(norms)//3}")
for i in range(0,len(norms),3):
    print(f"{norms[i]:.6f} {norms[i+1]:.6f} {norms[i+2]:.6f}")
