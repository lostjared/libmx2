#version 450
// Vertex-pulling fullscreen quad - no vertex buffer needed.
// Outputs tc at location 0 to be compatible with acidcam/library fragment shaders.

layout(location = 0) out vec2 tc;

void main() {
    // Fullscreen quad via gl_VertexIndex (draw 6 vertices, no VBO)
    const vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );
    const vec2 texcoords[4] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    const int indices[6] = int[](0, 1, 2, 0, 2, 3);
    int qi = indices[gl_VertexIndex];
    gl_Position = vec4(positions[qi], 0.0, 1.0);
    tc = texcoords[qi];
}
