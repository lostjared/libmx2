#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 spritePos;
    vec2 spriteSize;
    vec4 params;  // x=colorR, y=colorG, z=colorB, w=time
} pc;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec4 adjustHue(vec4 color, float angle) {
    float U = cos(angle);
    float W = sin(angle);
    mat3 rotationMatrix = mat3(
        0.299, 0.587, 0.114,
        0.299, 0.587, 0.114,
        0.299, 0.587, 0.114
    ) + mat3(
        0.701, -0.587, -0.114,
        -0.299, 0.413, -0.114,
        -0.3, -0.588, 0.886
    ) * U + mat3(
        0.168, 0.330, -0.497,
        -0.328, 0.035, 0.292,
        1.25, -1.05, -0.203
    ) * W;
    return vec4(rotationMatrix * color.rgb, color.a);
}

void main() {
    vec2 tc = fragTexCoord;
    vec2 iResolution = pc.spriteSize;
    float time_f = pc.params.w;

    // Bubble distortion effect
    vec2 buv = tc * 2.0 - 1.0;
    float len = length(buv);
    float time_t = pingPong(time_f, 10.0);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);

    vec2 distort = buv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    distort = sin(distort * time_t);

    vec4 texColor = texture(spriteTexture, distort * 0.5 + 0.5);
    vec4 bubbleColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);

    // Ripple + hue shift effect
    vec2 uv = (tc - 0.5) * iResolution / min(iResolution.x, iResolution.y);
    float dist = length(uv);
    float ripple = sin(dist * 12.0 - time_t * 10.0) * exp(-dist * 4.0);
    float hueShift = time_t * ripple * 2.0;
    outColor = adjustHue(bubbleColor, hueShift);
}
