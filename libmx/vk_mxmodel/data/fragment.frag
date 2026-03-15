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
    vec3 normal = normalize(fragNormal);
    vec4 texColor = texture(texSampler, fragTexCoord);

    vec3 albedo = texColor.rgb * fragColor;

    vec3 lightDir = normalize(vec3(0.35, 1.0, 0.55));
    vec3 lightColor = vec3(1.0);

    float ambientStrength = 0.20;
    vec3 ambient = ambientStrength * lightColor;

    float nDotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = nDotL * lightColor;

    vec3 cameraPos = inverse(ubo.view)[3].xyz;
    vec3 viewDir = normalize(cameraPos - fragWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float specPow = 48.0;
    float spec = pow(max(dot(normal, halfDir), 0.0), specPow);
    float specStrength = 0.25;
    vec3 specular = specStrength * spec * lightColor;

    vec3 litColor = (ambient + diffuse) * albedo + specular;
    vec3 result = litColor;
    outColor = vec4(result, texColor.a);
}
