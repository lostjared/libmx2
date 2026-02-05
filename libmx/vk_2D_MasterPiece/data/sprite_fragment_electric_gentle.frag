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

vec3 palette(float t) {
    vec3 a = vec3(0.5);
    vec3 b = vec3(0.5);
    vec3 c = vec3(1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    float time_f = pc.param0;
    vec2 uv = fragTexCoord;
    
    // Gentle swirl effect - very small displacement
    vec2 center = vec2(0.5);
    vec2 toCenter = uv - center;
    float dist = length(toCenter);
    float angle = atan(toCenter.y, toCenter.x);
    
    // Very gentle angle shift
    angle += sin(time_f * 0.3) * dist * 0.3;
    
    vec2 swirlUV = center + vec2(cos(angle), sin(angle)) * dist;
    
    // Sample texture
    vec3 texCol = texture(samp, swirlUV).rgb;
    
    // Electric color palette overlay
    vec3 pal = palette(dist * 2.0 + time_f * 0.5);
    
    // Mix texture with palette
    vec3 col = mix(texCol, pal, 0.3);
    
    // Add gentle pulsing glow
    float glow = 0.2 * (0.5 + 0.5 * sin(time_f * 0.6));
    col += pal * glow;
    
    outColor = vec4(col, 1.0);
}
