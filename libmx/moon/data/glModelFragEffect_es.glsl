#version 300 es
precision highp float;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

out vec4 FragColor;

uniform sampler2D modelTexture;
uniform float time;

// PingPong animation function
float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main() {
    // Normalize UV coordinates to -1 to 1 range
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float len = length(uv);
    
    // Use time for animation
    float time_t = pingPong(time, 10.0);
    
    // Create bubble outline with time-based animation
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);
    
    // Distortion with time-based sinusoidal wave
    vec2 distort = uv * (1.0 + 0.1 * sin(time + len * 20.0));
    distort = sin(distort * time_t);
    
    // Sample texture with distorted coordinates
    vec4 texColor = texture(modelTexture, distort * 0.5 + 0.5);
    
    // Standard lighting
    vec3 lightPos = vec3(0.0, 5.0, 0.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    
    float ambientStrength = 0.8;
    vec3 ambient = vec3(1.0) * ambientStrength;
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vec3(1.0) * diff;
    
    float specularStrength = 0.8;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(1.0) * (specularStrength * spec);
    
    float dist = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
    
    // Convert from sRGB to linear
    vec3 texLinear = pow(texColor.rgb, vec3(2.2));
    
    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = texLinear * lighting;
    
    // Mix with white based on bubble intensity (creates highlight)
    result = mix(result, vec3(1.0, 1.0, 1.0), bubble * 0.5);
    
    // Convert back to sRGB
    result = pow(result, vec3(1.0/2.2));
    FragColor = vec4(result, texColor.a);
    FragColor.rgb -= 0.3;
}
