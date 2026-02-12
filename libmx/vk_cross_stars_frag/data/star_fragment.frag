#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D starTexture;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

float pingPong(float x, float len) {
    float modVal = mod(x, len * 2.0);
    return modVal <= len ? modVal : len * 2.0 - modVal;
}

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(starTexture, gl_PointCoord);
    if(texColor.r < 0.5 && texColor.g < 0.5 && texColor.b < 0.5)
        discard;

    // params.x = time_f, params.w = alpha
    float time_f = ubo.params.x;
    float alpha = ubo.params.w;

    // Center UV so (0,0) is middle of the star sprite
    vec2 uv = gl_PointCoord - vec2(0.5);

    float t = time_f * 0.5;

    // Per-star phase offset derived from vertex color (unique per star)
    float phase = fract(fragColor.r * 7.13 + fragColor.g * 11.47 + fragColor.b * 5.31) * 6.2831;

    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t;

    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;

    float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831 + phase);
    float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230 + phase * 1.7);
    float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560 + phase * 2.3);

    // Strobe: each star pulses a unique hue over time
    float strobe = sin(time_f * 3.0 + phase) * 0.5 + 0.5;
    vec3 strobeColor = vec3(
        sin(time_f * 2.0 + phase) * 0.5 + 0.5,
        sin(time_f * 2.0 + phase + 2.094) * 0.5 + 0.5,
        sin(time_f * 2.0 + phase + 4.189) * 0.5 + 0.5
    );

    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    col = mix(col, strobeColor, strobe * 0.6);
    col = mix(col, texColor.rgb, 0.1);

    outColor = vec4(col, alpha * fragColor.a);
}
