#version 300 es
precision mediump float; 
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    localPos = aPos;  // Pass the position directly as texture coordinates
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}