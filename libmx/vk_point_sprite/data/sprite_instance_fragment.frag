#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragParams; 
layout(location = 2) in vec2 fragSpriteSize;

layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

void main() {
    vec2 tc = fragTexCoord;
    vec4 texSample = texture(spriteTexture, tc);
    vec3 texColor = texSample.rgb;
    float luminance = dot(texColor, vec3(0.299, 0.587, 0.114));
    float minChan  = min(t_exColor.r, min(texColor.g, texColor.b));
    float whiteness = smoothstep(0.70, 0.95, luminance) * smoothstep(0.55, 0.85, minChan);
    float alpha = 1.0 - whiteness;
    if (alpha < 0.05)
        discard;
    float time_f = fragParams.w;
    float pulse = sin(time_f * 3.0) * 0.15 + 0.85;
    vec3 tintColor = fragParams.xyz;
    vec3 col = texColor * tintColor * pulse;
    outColor = vec4(col, alpha);
}
