#version 330 core

in float intensity;
in float rotation;

uniform sampler2D snowflakeTexture;
uniform float time;

out vec4 fragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float angle = rotation + time * 0.5;
    float s = sin(angle);
    float c = cos(angle);
    vec2 rotatedCoord = vec2(
        coord.x * c - coord.y * s,
        coord.x * s + coord.y * c
    );
    rotatedCoord += vec2(0.5);
    vec4 texColor = texture(snowflakeTexture, rotatedCoord);
    if(texColor.r < 0.1) {
        discard;
    }
    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.4, dist) * intensity;
    vec3 color_white = vec3(1.0, 1.0, 1.0);
     fragColor = vec4(color_white, texColor.a * alpha);
}