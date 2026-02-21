#version 450
layout(location = 0) out vec4 color;
layout(location = 0) in vec2 tc;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 spritePos;
    vec2 spriteSize;
    vec4 params;  // x=colorR, y=colorG, z=colorB, w=time
} pc;

layout(binding = 0) uniform sampler2D samp;

// Uniform compatibility aliases
#define time_f pc.params.w
#define iResolution pc.spriteSize


float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

void main(void) {
    vec2 center = vec2(0.5);
    vec2 uv = tc - center;
    float r = length(uv);
    float t = time_f;
    float s = pingPong(t, 10.0) * 0.1;

    float bendR = 0.15 + 0.1*sin(t*0.5);
    float swirl = (0.35 + 0.25*sin(t*0.33)) * (1.0 - smoothstep(0.0, 0.707, r));
    float ang = atan(uv.y, uv.x) + swirl;
    float rb = r * (1.0 + bendR * sin(r*12.0 + t*1.7));

    vec2 n1 = vec2(cos(t*0.37), sin(t*0.37));
    vec2 n2 = vec2(cos(t*0.53+1.7), sin(t*0.53+1.7));
    float w1 = sin(dot(uv, n1)*18.0 + t*1.3);
    float w2 = sin(dot(uv, n2)*14.0 - t*1.1);
    vec2 dirBend = normalize(n1)*w1 + normalize(n2)*w2;

    vec2 uvb = vec2(cos(ang), sin(ang)) * rb;
    uvb += dirBend * (0.025 + 0.02*sin(t*0.21)) * (0.5 + 0.5*sin(r*10.0 + t));

    float rot = sin(t*3.14159265*0.2) * 0.6;
    mat2 R = mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
    uvb = R * uvb;

    uv = uvb + center;
    uv -= sin(uv*6.28318 + t) * (0.01 + 0.01*s);

    color = vec4(texture(samp, uv).rgb, pc.params.x);
}
