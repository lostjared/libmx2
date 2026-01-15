#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;

layout(binding = 1) uniform MyUniforms {
    float time;
    vec3 color;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
} ubo;

void main() {
    // Sample texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Normalize the normal
    vec3 norm = normalize(fragNormal);
    
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;
    
    // Diffuse lighting
    vec3 lightDir = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ubo.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * ubo.lightColor;
    
    // Combine all lighting
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    outColor = vec4(result, texColor.a);
}
