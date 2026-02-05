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

void main() {
    float time_f = pc.param0;
    vec2 uv = fragTexCoord;
    
    // Very gentle wave distortion - keeps UVs close to original
    float wave1 = sin(uv.y * 8.0 + time_f) * 0.01;
    float wave2 = cos(uv.x * 6.0 + time_f * 0.7) * 0.01;
    
    vec2 distortedUV = uv + vec2(wave1, wave2);
    
    // Sample texture
    vec3 col = texture(samp, distortedUV).rgb;
    
    // Gentle color enhancement
    col *= 1.2;
    col = mix(col, col.gbr, 0.1 * sin(time_f * 0.5));
    
    outColor = vec4(col, 1.0);
}
