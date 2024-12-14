#version 330 core
layout(location = 0) in vec3 position;   
layout(location = 1) in vec3 normal;     
layout(location = 2) in vec2 texCoord;   

out vec3 fragPos;       
out vec3 fragNormal;    
out vec2 fragTexCoord;  

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 viewPos4 = view * model * vec4(position, 1.0);
    fragPos = viewPos4.xyz;
    fragNormal = mat3(transpose(inverse(view * model))) * normal;
    fragTexCoord = texCoord;                                 
    gl_Position = projection * view * vec4(fragPos, 1.0);    
}