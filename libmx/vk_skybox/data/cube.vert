#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(push_constant) uniform MVP {
    mat4 mvp;
} constants;

layout(location = 0) out vec3 localPos;
layout(location = 1) out vec3 vertexColor;

void main()
{
    localPos = aPos;
    vertexColor = aColor;
    gl_Position = constants.mvp * vec4(aPos, 1.0);
}
