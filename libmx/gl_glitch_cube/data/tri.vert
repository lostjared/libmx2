#version 100
attribute vec3 position; 
attribute vec2 texCoord; 

uniform mat4 model;      
uniform mat4 view;       
uniform mat4 projection; 

varying vec2 vTexCoord; 

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vTexCoord = texCoord;
}