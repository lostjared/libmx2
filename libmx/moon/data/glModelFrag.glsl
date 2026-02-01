#version 330 core
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

out vec4 FragColor;

uniform sampler2D modelTexture;
uniform float time;

void main() {
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
    
    vec4 texColor = texture(modelTexture, fragTexCoord);
    // Convert from sRGB to linear
    vec3 texLinear = pow(texColor.rgb, vec3(2.2));
    
    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = texLinear * lighting;
    
    // Convert back to sRGB
    result = pow(result, vec3(1.0/2.2));
    FragColor = vec4(result, texColor.a);
    FragColor.rgb -= 0.3;
    // Use time to prevent uniform optimization (no visual effect)
    FragColor.rgb += time * 0.0;
}
