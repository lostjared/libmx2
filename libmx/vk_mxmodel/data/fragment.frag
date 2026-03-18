#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;  
    vec4 color;  
} ubo;

void main() {
    // Light position in view space (matches GL: world pos (0, 10, 5) transformed by view matrix)
    vec3 lightPosView = (ubo.view * vec4(0.0, 10.0, 5.0, 1.0)).xyz;
    vec3 lightColor = vec3(1.0);

    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPosView - fragPos);
    vec3 viewDir = normalize(-fragPos); // In view space, camera is at origin

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.2;
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 textureColor = texture(texSampler, fragTexCoord).rgb;
    vec3 result = (ambient + diffuse + specular) * textureColor;

    outColor = vec4(result, 1.0);
}
