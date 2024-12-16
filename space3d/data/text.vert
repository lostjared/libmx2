#version 300 es
precision mediump float; 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0); // Position directly in screen space
    TexCoord = aTexCoord;         // Pass texture coordinates to the fragment shader
}
