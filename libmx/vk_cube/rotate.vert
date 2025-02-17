#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = pc.model * vec4(aPos, 1.0);
    vColor = aColor;
}
