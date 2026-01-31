#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D starTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(starTexture, gl_PointCoord);
    if(texColor.r< 0.5 && texColor.g < 0.5 && texColor.b < 0.5)
        discard;
    outColor = texColor * fragColor;
}
