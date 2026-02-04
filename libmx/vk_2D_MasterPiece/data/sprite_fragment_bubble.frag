#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D samp;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 spritePos;
    vec2 spriteSize;
    vec4 params;  // x: time_f, y: unused, z: unused, w: unused
} pc;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float len = length(uv);
    float time_t = pingPong(pc.params.x, 10.0);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);
    
    vec2 distort = uv * (1.0 + 0.1 * sin(pc.params.x + len * 20.0));
    
    distort = sin(distort * time_t);
    
    vec4 texColor = texture(samp, distort * 0.5 + 0.5);
    outColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
}
