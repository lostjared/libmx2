#version 450
#extension GL_ARB_gpu_shader_fp64 : enable

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    double centerX;
    double centerY;
    double zoom;
    int maxIterations;
    float time;
} pc;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float smoothColor(int iter, dvec2 z) {
    float mag = float(dot(z, z));
    return float(iter) - log2(log2(max(mag, 1e-12))) + 4.0;
}

void main() {
    vec2 resolution = ubo.params.xy;
    dvec2 fragCoord = dvec2(gl_FragCoord.xy);

    dvec2 uv = (fragCoord - 0.5 * dvec2(resolution)) / double(min(resolution.x, resolution.y));
    dvec2 c = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);

    dvec2 z = dvec2(0.0);
    int iterations = 0;

    for (int i = 0; i < pc.maxIterations; i++) {
        if (dot(z, z) > 4.0) break;
        z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iterations++;
    }

    if (iterations == pc.maxIterations) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float sm = smoothColor(iterations, z);
    float angle = atan(float(z.y), float(z.x));
    float radius = sqrt(float(dot(z, z)));

    float spiral = angle / (2.0 * 3.14159265) + radius * 0.35 + sm * 0.02;
    float hue = fract(spiral + pc.time * 0.08);

    float sat = 0.8 + 0.2 * sin(sm * 0.12 + pc.time * 0.7);
    float val = 0.7 + 0.3 * sin(radius * 1.5 - pc.time * 0.9 + angle * 2.0);

    vec3 color = hsv2rgb(vec3(hue, clamp(sat, 0.0, 1.0), clamp(val, 0.0, 1.0)));

    float glow = exp(-radius * 0.35);
    color = mix(color, vec3(1.0, 0.9, 1.0), glow * 0.25);

    outColor = vec4(color, 1.0);
}
