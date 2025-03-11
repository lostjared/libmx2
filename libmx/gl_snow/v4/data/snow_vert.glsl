#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 attributes; // intensity, size, rotation

uniform mat4 viewProj;
uniform float time;

out float intensity;
out float rotation;

void main() {
    intensity = attributes.x;
    rotation = attributes.z;
    
    
    vec3 pos = position;
    pos.x += sin(time * 0.5 + position.y * 0.1) * 0.1 * position.z;
    
    
    float size = attributes.y * 20.0;
    float depth = (viewProj * vec4(pos, 1.0)).w;
    float pointSize = size * (400.0 / depth);
    
    
    gl_Position = viewProj * vec4(pos, 1.0);
    gl_PointSize = pointSize;
}