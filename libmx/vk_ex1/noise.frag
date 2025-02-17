#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    float time;
} pc;

// A simple pseudo-random generator function
float random(vec2 st) {
    return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    // Normalize the fragment coordinates (assuming an 800x600 resolution; 
    // ideally pass the resolution as a uniform)
    vec2 resolution = vec2(800.0, 600.0);
    vec2 uv = gl_FragCoord.xy / resolution;
    
    // Amplify uv to get more noticeable variation
    uv *= 10.0;
    // Add a scaled time offset so the noise visibly changes every frame
    uv += pc.time * 5.0;
    
    // Generate random color channels based on the modified uv coordinates
    float r = random(uv);
    float g = random(uv + vec2(1.0, 0.0));
    float b = random(uv + vec2(0.0, 1.0));
    
    outColor = vec4(r, g, b, 1.0);
}
