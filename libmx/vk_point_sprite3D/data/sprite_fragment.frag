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
    float colorR;      // params[0] - red tint
    float colorG;      // params[1] - green tint
    float colorB;      // params[2] - blue tint
    float brightness;  // params[3] - brightness multiplier / time_f
} pc;

float pingPong(float x, float len) {
    float modVal = mod(x, len * 2.0);
    return modVal <= len ? modVal : len * 2.0 - modVal;
}

void main() {
    vec2 tc = fragTexCoord;

    // Early discard for corners outside circular radius — saves all later work
    vec2 fromCenter = tc * 2.0 - 1.0;
    float edgeRadius = length(fromCenter);
    if (edgeRadius > 1.0)
        discard;

    float time_f = pc.brightness;
    vec2 iResolution = vec2(pc.spriteSizeW, pc.spriteSizeH);

    // Polar color swirl effect
    vec2 uv = (tc * iResolution - 0.5 * iResolution) / iResolution.y;
    float t = time_f * 0.5;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t;

    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;

    float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560);

    vec3 col = vec3(r, g, b) * 0.5 + 0.5;

    // Blend with sprite texture
    vec3 texColor = texture(spriteTexture, tc).rgb;
    col = mix(col, texColor, 0.3);

    // Apply per-orb color tint
    vec3 tintColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    if (tintColor.r + tintColor.g + tintColor.b >= 0.01) {
        col *= tintColor;
    }

    // Key out white pixels
    float luminance = dot(col, vec3(0.299, 0.587, 0.114));
    float whiteness = smoothstep(0.75, 1.0, luminance);
    float alpha = 1.0 - whiteness;

    // Circular edge fade for soft round orbs
    float edgeFade = smoothstep(1.0, 0.7, edgeRadius);
    alpha *= edgeFade;

    if (alpha < 0.05)
        discard;

    outColor = vec4(col, alpha);
}
