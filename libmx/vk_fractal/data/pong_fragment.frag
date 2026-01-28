#version 450

layout(location = 0) in vec3 fragColor;    
layout(location = 1) in vec2 fragTexCoord; 
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

// UBO with same structure as base class for descriptor set compatibility
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;  
    vec4 color;
} ubo;

// Push constants for per-object model matrix and color
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 color;
} pc;

void main() {
    vec3 lightPos = vec3(0.0, 5.0, 5.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    
    float ambientStrength = 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Sample texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Blend texture with tint color (fragColor from push constants)
    vec3 tintedTex = texColor.rgb * fragColor;
    
    // Apply lighting
    vec3 ambient = ambientStrength * tintedTex;
    vec3 diffuse = diff * tintedTex;
    vec3 lighting = ambient + diffuse + specular;
    
    outColor = vec4(lighting, 1.0);
}
