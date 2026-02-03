#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1; 
    float padding2;
    float params[4]; // params[3] is Time, params[2] is Brightness
} pc;

void main(void) {
    float time = pc.params[3];
    float brightness = pc.params[2];
    float flicker = sin(time * 100.0) > 0.95 ? 1.0 : 0.0;
    float pulse = pow(0.5 + 0.5 * sin(time * 5.0), 3.0);
    float rOffset = flicker * 0.02 * sin(time * 150.0);
    float bOffset = flicker * 0.02 * cos(time * 150.0);
    vec2 uv = fragTexCoord;
    uv.x += flicker * 0.05 * sin(time * 200.0); // Horizontal glitch
    float r = texture(spriteTexture, uv + vec2(rOffset, 0.0)).r;
    float g = texture(spriteTexture, uv).g;
    float b = texture(spriteTexture, uv - vec2(bOffset, 0.0)).b;
    float a = texture(spriteTexture, uv).a;
    vec4 texColor = vec4(r, g, b, a);
    float scanline = sin(fragTexCoord.y * 30.0) * 0.15 + 0.85;
    vec3 color = texColor.rgb * (0.3 + pulse * 2.2 + flicker * 0.7);
    color *= vec3(0.95, 1.05, 1.0);
    outColor = vec4(color * brightness * scanline, texColor.a);
}