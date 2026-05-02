#version 450

layout(location = 0) in vec3 fragColor;    
layout(location = 1) in vec2 fragTexCoord; 
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
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
    vec3 lightPos = vec3(0.0, 5.0, 0.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * fragColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * fragColor;
    float specularStrength = 0.8;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    float distance = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = texColor.rgb * lighting;
    outColor = vec4(result, texColor.a);
}
