#version 330

in vec2 vTexCoord;
out vec4 color;
uniform sampler2D texture1;
uniform float alpha;

void main(void) {
    float time_f = alpha;
    vec2 tc = vTexCoord;
    vec2 center = vec2(0.5, 0.5);
    vec2 uv = tc - center;
    float dist = length(uv);
    float ripple = sin(10.0 * dist - time_f * 6.28318) * 0.1;
    uv += uv * ripple;
    uv += center;
    color = texture(texture1, uv);
}
