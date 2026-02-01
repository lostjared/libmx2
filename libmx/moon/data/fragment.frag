#version 450

layout(set = 0, binding = 1, std140) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 3) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 lightPos = vec3(0.0, 5.0, 0.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    float ambientStrength = 0.800000011920928955078125;
    vec3 ambient = fragColor * ambientStrength;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = fragColor * diff;
    float specularStrength = 0.800000011920928955078125;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(1.0) * (specularStrength * spec);
    float _distance = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / ((1.0 + (0.100000001490116119384765625 * _distance)) + ((0.00999999977648258209228515625 * _distance) * _distance));
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 lighting = ambient + ((diffuse + specular) * attenuation);
    vec3 result = texColor.xyz * lighting;
    outColor = vec4(result, texColor.w);
}
