#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D samp;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1;
    float padding2;
    float param0;  // time_f
    float param1;
    float param2;
    float param3;
} pc;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

void main() {
    vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);
    float time_f = pc.param0;
    
    vec2 uv = fragTexCoord;
    float aspect = iResolution.x / iResolution.y;
    vec2 centered = (uv - 0.5) * vec2(aspect, 1.0);
    
    float t = time_f * 0.5;
    
    // Gentle spiral warp
    float dist = length(centered);
    float angle = atan(centered.y, centered.x);
    
    float warpAmount = 0.3 * (noise(centered * 2.0 + t * 0.2) - 0.5);
    angle += warpAmount + dist * sin(t * 0.3) * 0.5;
    
    vec2 warped = vec2(cos(angle), sin(angle)) * dist;
    
    // Add flowing noise offset (kept small to avoid huge UV jumps)
    vec2 flow = vec2(
        noise(warped * 1.5 + t * 0.1) - 0.5,
        noise(warped * 1.5 + vec2(50.0, 30.0) - t * 0.15) - 0.5
    ) * 0.2;
    
    // Convert back to UV space for texture sampling
    vec2 finalPos = warped + flow;
    vec2 texUV = finalPos / vec2(aspect, 1.0) + 0.5;
    
    // Sample with chromatic aberration
    float aberration = 0.002;
    vec2 dir = normalize(centered) * aberration;
    
    float r = texture(samp, texUV + dir).r;
    float g = texture(samp, texUV).g;
    float b = texture(samp, texUV - dir).b;
    
    vec3 col = vec3(r, g, b);
    
    // Enhance colors
    float enhance = 1.2 + 0.3 * sin(t * 0.4);
    col *= enhance;
    
    // Add some color shift
    col = mix(col, col.brg, 0.2 * (sin(t * 0.6) * 0.5 + 0.5));
    
    outColor = vec4(col, 1.0);
}
