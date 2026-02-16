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

    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * fragColor;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * fragColor;

    // Specular (strong for glass)
    float specularStrength = 1.5;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);

    // Fresnel-like rim effect for glass
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);

    float distance = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = texColor.rgb * lighting + fresnel * vec3(0.3, 0.5, 0.7);

    // Transparency: base alpha from color uniform, boosted at edges (fresnel)
    float alpha = ubo.color.a * (0.3 + 0.7 * fresnel) * texColor.a;

    outColor = vec4(result, alpha);
}
