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

float rainbowHue(vec2 uv) {
    float angle = atan(uv.y, uv.x) / (2.0 * 3.14159265359);
    float radius = length(uv);
    return fract(angle + radius * 0.5);
}


vec3 rainbowEffect(float iterations, float smoothIter, vec2 uv) {
    float hue1 = fract(smoothIter * 0.015);
    float hue2 = rainbowHue(uv);
    float hue3 = fract(hue1 + pc.time * 0.15);
    float finalHue = fract(hue3 * 0.6 + hue2 * 0.3 + hue1 * 0.1);
    float saturation = 0.95;
    float value = 0.85 + 0.15 * sin(smoothIter * 0.3);
    return hsv2rgb(vec3(finalHue, saturation, value));
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
        float hue = fract(pc.time * 0.05 + float(length(uv)) * 0.2);
        vec3 color = hsv2rgb(vec3(hue, 0.7, 0.3));
        outColor = vec4(color, 1.0);
    } else {
        float zMag = float(dot(z, z));
        float smoothIter = float(iterations) - log2(log2(zMag)) + 4.0;
        vec3 color = rainbowEffect(float(iterations), smoothIter, vec2(uv));
        outColor = vec4(color, 1.0);
    }
}
