#version 450
#extension GL_ARB_gpu_shader_fp64 : enable

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube cubemapTexture;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x=time, y=width, z=height
} ubo;

layout(push_constant) uniform PushConstants {
    double centerX;
    double centerY;
    double zoom;
    int maxIterations;
    float time;
} pc;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 mandelbrot(vec2 uv) {
    // Map face UV to complex plane with double precision
    dvec2 c = dvec2(uv) / pc.zoom + dvec2(pc.centerX, pc.centerY);

    // Mandelbrot iteration
    dvec2 z = dvec2(0.0);
    int iterations = 0;

    for (int i = 0; i < pc.maxIterations; i++) {
        if (dot(z, z) > 4.0) break;
        z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iterations++;
    }

    if (iterations == pc.maxIterations) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        float zMag = float(dot(z, z));
        float smoothIter = float(iterations) - log2(log2(zMag)) + 4.0;
        float hue = fract(smoothIter * 0.02 + pc.time * 0.1);
        float saturation = 0.8;
        float value = 1.0;
        vec3 color = hsv2rgb(vec3(hue, saturation, value));
        return vec4(color, 1.0);
    }
}

void main() {
    vec3 tc = normalize(localPos);

    // Derive a 2D UV from the cube face local position
    vec3 absTC = abs(tc);
    vec2 faceUV;
    if (absTC.x >= absTC.y && absTC.x >= absTC.z) {
        faceUV = tc.yz / absTC.x;
    } else if (absTC.y >= absTC.x && absTC.y >= absTC.z) {
        faceUV = tc.xz / absTC.y;
    } else {
        faceUV = tc.xy / absTC.z;
    }

    // Draw the Mandelbrot fractal on each face (mirrored for seamless edges)
    vec2 uv = abs(faceUV);
    vec2 duv = 0.5 * fwidth(uv);
    vec4 f0 = mandelbrot(uv);
    vec4 f1 = mandelbrot(uv + vec2(duv.x, 0.0));
    vec4 f2 = mandelbrot(uv + vec2(0.0, duv.y));
    vec4 f3 = mandelbrot(uv + duv);
    vec4 fractalColor = (f0 + f1 + f2 + f3) * 0.25;

    // Sample the cubemap texture with mirrored coordinates so faces reflect into each other
    vec3 mirrorTC = abs(tc);
    vec4 texColor = texture(cubemapTexture, mirrorTC);

    // Blend cubemap only where fractal is near-black
    float lum = max(fractalColor.r, max(fractalColor.g, fractalColor.b));
    float blendFactor = 1.0 - smoothstep(0.02, 0.08, lum);
    vec4 blended = mix(fractalColor, texColor, blendFactor);

    // Magical color overlay
    float sparkle = abs(sin(ubo.params.x * 10.0 + tc.x * 100.0) * cos(ubo.params.x * 15.0 + tc.y * 100.0));
    vec3 magicalColor = vec3(
        sin(ubo.params.x * 2.0) * 0.5 + 0.5,
        cos(ubo.params.x * 3.0) * 0.5 + 0.5,
        sin(ubo.params.x * 4.0) * 0.5 + 0.5
    );

    outColor = blended + vec4(magicalColor * sparkle * 0.3, 0.0);
}
