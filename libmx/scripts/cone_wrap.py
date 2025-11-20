import math

segments = 696
radius = 0.5
half_h = 0.5

verts = []
tex = []
norms = []

tip = (0.0, 0.0, half_h)
base_center = (0.0, 0.0, -half_h)

def normalize(v):
    x,y,z = v
    l = math.sqrt(x*x + y*y + z*z)
    if l == 0: return (0.0, 0.0, 1.0)
    return (x/l, y/l, z/l)

for i in range(segments):
    a0 = (i / segments) * math.tau
    a1 = ((i + 1) / segments) * math.tau
    x0 = math.cos(a0) * radius
    y0 = math.sin(a0) * radius
    x1 = math.cos(a1) * radius
    y1 = math.sin(a1) * radius

    v0 = (x0, y0, -half_h)
    v1 = (x1, y1, -half_h)

    verts.append(tip)
    verts.append(v0)
    verts.append(v1)

    u0 = i / segments
    u1 = (i + 1) / segments
    tex.append((u0, 0.0))
    tex.append((u0, 1.0))
    tex.append((u1, 1.0))

    norms.append(normalize((x0, y0, half_h)))
    norms.append(normalize(v0))
    norms.append(normalize(v1))

for i in range(segments):
    a0 = (i / segments) * math.tau
    a1 = ((i + 1) / segments) * math.tau
    x0 = math.cos(a0) * radius
    y0 = math.sin(a0) * radius
    x1 = math.cos(a1) * radius
    y1 = math.sin(a1) * radius

    v0 = (x0, y0, -half_h)
    v1 = (x1, y1, -half_h)

    verts.append(base_center)
    verts.append(v1)
    verts.append(v0)

    tex.append((0.5, 0.5))
    tex.append((1.0, 1.0))
    tex.append((0.0, 1.0))

    norms.append((0.0, 0.0, -1.0))
    norms.append((0.0, 0.0, -1.0))
    norms.append((0.0, 0.0, -1.0))

print("tri 0 0")
print("vert", len(verts))
for v in verts:
    print(v[0], v[1], v[2])

print("\ntex", len(tex))
for t in tex:
    print(t[0], t[1])

print("\nnorm", len(norms))
for n in norms:
    print(n[0], n[1], n[2])
