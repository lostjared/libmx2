#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
} fs_in;

out vec4 FragColor;

uniform samplerCube cubemap1;
uniform samplerCube cubemap2;
uniform float blendFactor;
uniform sampler2D texture1;

void main()
{
    // Normalize reflection direction
    vec3 viewDir = normalize(fs_in.FragPos);
    vec3 reflectDir = reflect(viewDir, normalize(fs_in.Normal));
    
    // Sample from both cubemaps
    vec3 color1 = texture(cubemap1, reflectDir).rgb;
    vec3 color2 = texture(cubemap2, reflectDir).rgb;
    
    // Blend between cubemaps
    vec3 blendedCubemap = mix(color1, color2, blendFactor);
    
    // Sample base texture
    vec3 baseColor = texture(texture1, fs_in.TexCoords).rgb;
    
    // Combine with base texture
    vec3 finalColor = mix(baseColor, blendedCubemap, 0.3);
    
    FragColor = vec4(finalColor, 1.0);
}
