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

// PingPong animation function
float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

// Bubble effect from 330 core shader adapted to 450 core
void main()
{
    // Normalize UV coordinates to -1 to 1 range
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float len = length(uv);
    
    // Use params.x for time if available, otherwise use constant animation
    float time_t = pingPong(ubo.params.x, 10.0);
    
    // Create bubble outline with time-based animation
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);
    
    // Distortion with time-based sinusoidal wave
    vec2 distort = uv * (1.0 + 0.1 * sin(ubo.params.x + len * 20.0));
    distort = sin(distort * time_t);
    
    // Sample texture with distorted coordinates
    vec4 texColor = texture(texSampler, distort * 0.5 + 0.5);
    
    // Apply standard lighting
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
    
    vec3 lighting = ambient + ((diffuse + specular) * attenuation);
    vec3 result = texColor.xyz * lighting;
    
    // Mix with white based on bubble intensity (creates highlight)
    result = mix(result, vec3(1.0, 1.0, 1.0), bubble * 0.5);
    
    // Apply lens/magnifying glass distortion effect with chromatic aberration
    vec2 iResolution = vec2(1280.0, 720.0); // Default resolution, adjust as needed
    if (ubo.params.z > 0.0) { // Use params.z to control effect intensity
        float radius = 1.0;
        vec2 center = iResolution * 0.5;
        vec2 texCoord = fragTexCoord * iResolution;
        vec2 delta = texCoord - center;
        float dist = length(delta);
        float maxRadius = min(iResolution.x, iResolution.y) * radius;

        if (dist < maxRadius) {
            float scaleFactor = 1.0 - pow(dist / maxRadius, 2.0);
            vec2 direction = normalize(delta);
            float offsetR = 0.015 * ubo.params.z;
            float offsetG = 0.0;
            float offsetB = -0.015 * ubo.params.z;

            vec2 texCoordR = center + delta * scaleFactor + direction * offsetR * maxRadius;
            vec2 texCoordG = center + delta * scaleFactor + direction * offsetG * maxRadius;
            vec2 texCoordB = center + delta * scaleFactor + direction * offsetB * maxRadius;

            texCoordR /= iResolution;
            texCoordG /= iResolution;
            texCoordB /= iResolution;
            texCoordR = clamp(texCoordR, 0.0, 1.0);
            texCoordG = clamp(texCoordG, 0.0, 1.0);
            texCoordB = clamp(texCoordB, 0.0, 1.0);
            
            float r = texture(texSampler, texCoordR).r;
            float g = texture(texSampler, texCoordG).g;
            float b = texture(texSampler, texCoordB).b;
            
            // Apply lens effect to result
            result = mix(result, vec3(r, g, b) * lighting, 0.6);
        }
    }
    
    outColor = vec4(result, texColor.w);
}
