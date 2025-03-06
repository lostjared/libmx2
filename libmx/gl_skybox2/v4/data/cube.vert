#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    localPos = aPos;  // Use position as texture coordinates
    
    // Force z coordinate to be 1.0 (maximum depth) after projection
    vec4 pos = projection * view * model * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Force depth to be maximum value
}