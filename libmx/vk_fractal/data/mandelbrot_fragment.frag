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

void main() {
    vec2 resolution = ubo.params.xy;
    dvec2 fragCoord = dvec2(gl_FragCoord.xy);
    
    // Map pixel coordinates to complex plane (using double precision)
    dvec2 uv = (fragCoord - 0.5 * dvec2(resolution)) / double(min(resolution.x, resolution.y));
    
    // Apply zoom and center offset
    dvec2 c = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);
    
    // Mandelbrot iteration (double precision)
    dvec2 z = dvec2(0.0);
    int iterations = 0;
    
    for (int i = 0; i < pc.maxIterations; i++) {
        if (dot(z, z) > 4.0) break;
        
        // z = z^2 + c
        z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iterations++;
    }
    
    // Coloring (back to float for output)
    if (iterations == pc.maxIterations) {
        // Inside the set - black
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Smooth coloring with escape time
        float zMag = float(dot(z, z));
        float smoothIter = float(iterations) - log2(log2(zMag)) + 4.0;
        
        // Create vibrant colors using HSV
        float hue = fract(smoothIter * 0.02 + pc.time * 0.1);
        float saturation = 0.8;
        float value = 1.0;
        
        vec3 color = hsv2rgb(vec3(hue, saturation, value));
        outColor = vec4(color, 1.0);
    }
}
