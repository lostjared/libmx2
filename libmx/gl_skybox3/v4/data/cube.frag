#version 330 core

in vec3 localPos;         // from your VS: direction into the cubemap
out vec4 FragColor;

uniform samplerCube cubemapTexture;
uniform float time_f;

const float PI = 3.14159265359;

// 2×2 rotation matrix
mat2 rot(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // 1) start with normalized cubemap direction
    vec3 dir = normalize(localPos);

    // 2) project onto 2D for your swirl/kaleido effect
    vec2 uv = dir.xy;

    // swirl amount (strongest at center)
    float radius = length(uv);
    float angle  = atan(uv.y, uv.x);
    float swirl  = sin(time_f * 2.0) * (1.0 - radius);
    angle += swirl;

    // kaleidoscope segmentation
    float sides   = 12.0;
    float segment = PI * 2.0 / sides;
    angle = mod(angle, segment);
    angle = abs(angle - segment * 0.5);

    // rebuild warped UV and apply a slow rotation
    vec2 p = vec2(cos(angle), sin(angle)) * radius;
    p = rot(time_f * 0.1) * p;

    // 3) reconstruct a 3D direction and sample
    vec3 warpedDir = normalize(vec3(p, dir.z));
    FragColor = texture(cubemapTexture, warpedDir);
}
