#version 450

layout(location = 0) in vec4 fragColor; 
layout(location = 1) in float twinkleFactor; 
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D starTexture;

void main() {
    vec4 texColor = texture(starTexture, gl_PointCoord);    
    if (texColor.a < 0.01) {
        discard;
    }
    vec3 finalRGB = texColor.rgb * fragColor.rgb * (0.8 + 0.2 * twinkleFactor);
    float finalAlpha = texColor.a * fragColor.a;
    outColor = vec4(finalRGB, finalAlpha);
}