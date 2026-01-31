#version 450

layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    float u = fragTexCoord.x;
    float v = fragTexCoord.y;
    float t = u * 3.14159 * 2.0;
    float r = sin(t + v * 3.14159);
    float g = sin(t * 0.8 + v * 2.0 + 2.0);
    float b = sin(t * 1.2 + v * 1.5 + 4.0);
    vec3 col = 0.5 + 0.5 * vec3(r, g, b);
    outColor = vec4(col, 1.0);
}
