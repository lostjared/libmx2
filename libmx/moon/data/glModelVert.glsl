#version 330 core
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;

void main() {
    fragTexCoord = inTexCoord;
    fragNormal = mat3(model) * inNormal;
    fragWorldPos = (model * vec4(inPosition, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
