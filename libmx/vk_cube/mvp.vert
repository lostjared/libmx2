#version 450
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) out vec3 vColor;

void main() {
    gl_Position = pc.mvp * vec4(aPos, 1.0);
    vColor = aColor;
}
