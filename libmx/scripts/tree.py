import math
import random

def generate_christmas_tree(levels=5, branches=8, height=5, radius=2):
    output = []

    # Triangle type: GL_TRIANGLES
    output.append("tri 0 0")

    # Generate vertices
    vertices = []
    normals = []
    tex_coords = []

    for level in range(levels):
        level_height = level * height / levels
        next_level_height = (level + 1) * height / levels
        current_radius = radius * (1 - level / levels)

        # Lego block dimensions for the current level
        block_width = current_radius / branches * 2
        block_height = height / levels / 2

        for i in range(branches):
            angle = i * (2 * math.pi / branches)
            x = current_radius * math.cos(angle)
            z = current_radius * math.sin(angle)

            # Vertices of the block (top and bottom faces of the Lego block)
            v1 = (x - block_width / 2, level_height, z - block_width / 2)
            v2 = (x + block_width / 2, level_height, z - block_width / 2)
            v3 = (x + block_width / 2, level_height, z + block_width / 2)
            v4 = (x - block_width / 2, level_height, z + block_width / 2)

            v5 = (x - block_width / 2, level_height + block_height, z - block_width / 2)
            v6 = (x + block_width / 2, level_height + block_height, z - block_width / 2)
            v7 = (x + block_width / 2, level_height + block_height, z + block_width / 2)
            v8 = (x - block_width / 2, level_height + block_height, z + block_width / 2)

            # Add vertices for the six faces of the Lego block
            vertices.extend([v1, v2, v3, v1, v3, v4])  # Bottom face
            vertices.extend([v5, v6, v7, v5, v7, v8])  # Top face
            vertices.extend([v1, v2, v6, v1, v6, v5])  # Front face
            vertices.extend([v2, v3, v7, v2, v7, v6])  # Right face
            vertices.extend([v3, v4, v8, v3, v8, v7])  # Back face
            vertices.extend([v4, v1, v5, v4, v5, v8])  # Left face

            # Normals (outward-facing for each face)
            normals.extend([
                (0, -1, 0), (0, -1, 0), (0, -1, 0),  # Bottom
                (0, 1, 0), (0, 1, 0), (0, 1, 0),  # Top
                (0, 0, -1), (0, 0, -1), (0, 0, -1),  # Front
                (1, 0, 0), (1, 0, 0), (1, 0, 0),  # Right
                (0, 0, 1), (0, 0, 1), (0, 0, 1),  # Back
                (-1, 0, 0), (-1, 0, 0), (-1, 0, 0),  # Left
            ])

            # Texture coordinates
            tex_coords.extend([
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Bottom
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Top
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Front
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Right
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Back
                (0, 0), (1, 0), (1, 1), (0, 0), (1, 1), (0, 1),  # Left
            ])

    output.append(f"vert {len(vertices)}")
    for v in vertices:
        output.append(f"{v[0]} {v[1]} {v[2]}")

    output.append(f"tex {len(tex_coords)}")
    for t in tex_coords:
        output.append(f"{t[0]} {t[1]}")

    output.append(f"norm {len(normals)}")
    for n in normals:
        output.append(f"{n[0]} {n[1]} {n[2]}")

    return "\n".join(output)

# Save the output to a file
def save_to_file(filename, data):
    with open(filename, "w") as file:
        file.write(data)

# Generate the Christmas tree and save it
filename = "lego_christmas_tree.txt"
data = generate_christmas_tree(levels=6, branches=12, height=10, radius=3)
save_to_file(filename, data)
print(f"Lego-style Christmas tree saved to {filename}")

