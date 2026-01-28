#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Blue (left) to Purple (right) gradient across x
    vec3 blue = vec3(0.1, 0.2, 0.6);
    vec3 purple = vec3(0.5, 0.1, 0.6);
    
    vec3 color = mix(blue, purple, fragCoord.x);
    outColor = vec4(color, 1.0);
}
