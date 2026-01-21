#version 330 core

in VS_OUT {
    vec2 TexCoords;
} fs_in;

out vec4 FragColor;

uniform sampler2D texture1;
uniform float aberrationStrength;

void main()
{
    // Chromatic aberration - shift RGB channels
    vec2 centerCoord = fs_in.TexCoords - vec2(0.5);
    
    // Red channel - shift outward
    vec3 redColor = texture(texture1, fs_in.TexCoords + centerCoord * aberrationStrength * 0.02).rgb;
    
    // Green channel - no shift
    vec3 greenColor = texture(texture1, fs_in.TexCoords).rgb;
    
    // Blue channel - shift inward
    vec3 blueColor = texture(texture1, fs_in.TexCoords - centerCoord * aberrationStrength * 0.02).rgb;
    
    // Combine channels
    vec3 finalColor = vec3(redColor.r, greenColor.g, blueColor.b);
    
    FragColor = vec4(finalColor, 1.0);
}
