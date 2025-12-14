import math

FILENAME = "hourglass_pyramid.mxmod"

TOTAL_HEIGHT = 40.0
PYRAMID_HEIGHT = 10.0
WAIST_RADIUS = 2.0
END_WIDTH = 15.0

RINGS_VERTICAL = 64
SEGMENTS_AROUND = 32

def generate_file():
    vertices = []
    uvs = []
    normals = []

    def add_quad(v1, v2, v3, v4, inward=True):
        if inward:
            vertices.extend([v4['pos'], v3['pos'], v2['pos']])
            uvs.extend([v4['uv'], v3['uv'], v2['uv']])
            normals.extend([v4['norm'], v3['norm'], v2['norm']])

            vertices.extend([v4['pos'], v2['pos'], v1['pos']])
            uvs.extend([v4['uv'], v2['uv'], v1['uv']])
            normals.extend([v4['norm'], v2['norm'], v1['norm']])
        else:
            vertices.extend([v1['pos'], v2['pos'], v3['pos']])
            uvs.extend([v1['uv'], v2['uv'], v3['uv']])
            normals.extend([v1['norm'], v2['norm'], v3['norm']])

            vertices.extend([v1['pos'], v3['pos'], v4['pos']])
            uvs.extend([v1['uv'], v3['uv'], v4['uv']])
            normals.extend([v1['norm'], v3['norm'], v4['norm']])

    def add_tri(v1, v2, v3, inward=True):
        if inward:
            vertices.extend([v3['pos'], v2['pos'], v1['pos']])
            uvs.extend([v3['uv'], v2['uv'], v1['uv']])
            normals.extend([v3['norm'], v2['norm'], v1['norm']])
        else:
            vertices.extend([v1['pos'], v2['pos'], v3['pos']])
            uvs.extend([v1['uv'], v2['uv'], v3['uv']])
            normals.extend([v1['norm'], v2['norm'], v3['norm']])

    def get_shape_radius(theta, blend):
        cos_t = math.cos(theta)
        sin_t = math.sin(theta)
        denom = max(abs(cos_t), abs(sin_t))
        if denom == 0:
            denom = 0.001
        r_circle = 1.0
        r_square = 1.0 / denom
        return r_circle * (1.0 - blend) + r_square * blend

    def get_hourglass_scale(y):
        half_h = TOTAL_HEIGHT / 2.0
        norm_y = abs(y) / half_h
        return WAIST_RADIUS + (END_WIDTH - WAIST_RADIUS) * (norm_y * norm_y)

    def make_vert(scale, shape_r, theta, y, u, v):
        rr = scale * shape_r
        x = rr * math.cos(theta)
        z = rr * math.sin(theta)
        nx = -math.cos(theta)
        nz = -math.sin(theta)
        return {'pos': (x, y, z), 'uv': (u, v), 'norm': (nx, 0.0, nz)}

    y_start = -TOTAL_HEIGHT / 2.0

    for r in range(RINGS_VERTICAL):
        v_curr = float(r) / RINGS_VERTICAL
        v_next = float(r + 1) / RINGS_VERTICAL

        y_curr = y_start + (v_curr * TOTAL_HEIGHT)
        y_next = y_start + (v_next * TOTAL_HEIGHT)

        blend_curr = abs(y_curr) / (TOTAL_HEIGHT / 2.0)
        blend_next = abs(y_next) / (TOTAL_HEIGHT / 2.0)

        scale_curr = get_hourglass_scale(y_curr)
        scale_next = get_hourglass_scale(y_next)

        ring_curr = []
        ring_next = []

        for s in range(SEGMENTS_AROUND + 1):
            u = float(s) / SEGMENTS_AROUND
            theta = 2.0 * math.pi * u

            r_c = get_shape_radius(theta, blend_curr)
            r_n = get_shape_radius(theta, blend_next)

            ring_curr.append(make_vert(scale_curr, r_c, theta, y_curr, u, v_curr))
            ring_next.append(make_vert(scale_next, r_n, theta, y_next, u, v_next))

        for s in range(SEGMENTS_AROUND):
            p1 = ring_curr[s]
            p2 = ring_next[s]
            p3 = ring_next[s + 1]
            p4 = ring_curr[s + 1]
            add_quad(p1, p2, p3, p4, inward=True)

    top_y_base = TOTAL_HEIGHT / 2.0
    top_y_tip = top_y_base + PYRAMID_HEIGHT
    top_tip = {'pos': (0.0, top_y_tip, 0.0), 'uv': (0.5, 0.0), 'norm': (0.0, -1.0, 0.0)}

    btm_y_base = -TOTAL_HEIGHT / 2.0
    btm_y_tip = btm_y_base - PYRAMID_HEIGHT
    btm_tip = {'pos': (0.0, btm_y_tip, 0.0), 'uv': (0.5, 0.0), 'norm': (0.0, 1.0, 0.0)}

    def generate_cap(y_base, tip_vert, is_top):
        scale = get_hourglass_scale(y_base)
        blend = 1.0

        base_ring = []
        for s in range(SEGMENTS_AROUND + 1):
            u = float(s) / SEGMENTS_AROUND
            theta = 2.0 * math.pi * u
            rr = get_shape_radius(theta, blend)
            x = scale * rr * math.cos(theta)
            z = scale * rr * math.sin(theta)
            base_ring.append({'pos': (x, y_base, z), 'uv': (u, 1.0), 'norm': (0.0, 0.0, 0.0)})

        for s in range(SEGMENTS_AROUND):
            v1 = base_ring[s]
            v2 = base_ring[s + 1]
            if is_top:
                add_tri(v2, v1, tip_vert, inward=True)
            else:
                add_tri(v1, v2, tip_vert, inward=True)

    generate_cap(top_y_base, top_tip, is_top=True)
    generate_cap(btm_y_base, btm_tip, is_top=False)

    with open(FILENAME, 'w') as f:
        count = len(vertices)
        f.write("tri 0 0\n")
        f.write(f"vert {count}\n")
        for x, y, z in vertices:
            f.write(f"{x:.6f} {y:.6f} {z:.6f}\n")
        f.write("\n")
        f.write(f"tex {count}\n")
        for u, v in uvs:
            f.write(f"{u:.6f} {v:.6f}\n")
        f.write("\n")
        f.write(f"norm {count}\n")
        for nx, ny, nz in normals:
            f.write(f"{nx:.6f} {ny:.6f} {nz:.6f}\n")

    print(f"Successfully generated {FILENAME} with {count} vertices.")

if __name__ == "__main__":
    generate_file()
