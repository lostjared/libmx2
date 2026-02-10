#version 450

layout(location = 0) in vec3 localPos;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec4 FragColor;

void main(void) {
    float time = localPos.x * 0.1;
    FragColor = vec4(vertexColor * (0.5 + 0.5 * sin(time)), 1.0);
}
