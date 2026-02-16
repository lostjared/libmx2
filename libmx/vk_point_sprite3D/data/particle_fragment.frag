#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // Create circular particle with soft edge
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    if (dist > 0.5) {
        discard;
    }
    
    // Soft edge falloff
    float alpha = fragColor.a * (1.0 - smoothstep(0.3, 0.5, dist));
    
    // Add glow effect
    vec3 glowColor = fragColor.rgb * 1.5;
    
    outColor = vec4(glowColor, alpha);
}
