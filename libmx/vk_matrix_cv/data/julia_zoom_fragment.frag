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
    
    // Apply zoom and center offset - this becomes our starting z
    dvec2 z = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);
    
    // Julia set constant - animated for interesting patterns
    // Classic Julia set constants that produce beautiful fractals:
    // (-0.7, 0.27015), (-0.4, 0.6), (0.285, 0.01), (-0.8, 0.156)
    double cReal = -0.7 + 0.15 * double(sin(pc.time * 0.3));
    double cImag = 0.27015 + 0.1 * double(cos(pc.time * 0.23));
    dvec2 c = dvec2(cReal, cImag);
    
    // Julia iteration (double precision)
    int iterations = 0;
    double zMagSq = 0.0;
    
    for (int i = 0; i < pc.maxIterations; i++) {
        zMagSq = z.x * z.x + z.y * z.y;
        if (zMagSq > 4.0) break;
        
        // z = z^2 + c
        z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iterations++;
    }
    
    // Coloring (back to float for output)
    if (iterations == pc.maxIterations) {
        // Inside the set - deep color based on final position
        float depth = float(zMagSq) * 0.1;
        vec3 insideColor = hsv2rgb(vec3(0.6 + depth * 0.1, 0.8, 0.15 + depth * 0.05));
        outColor = vec4(insideColor, 1.0);
    } else {
        // Smooth coloring with escape time
        float zMag = float(z.x * z.x + z.y * z.y);
        float smoothIter = float(iterations) - log2(max(log2(zMag), 1.0)) + 4.0;
        
        // Create vibrant colors using HSV with time-based animation
        float hue = fract(smoothIter * 0.025 + pc.time * 0.05);
        float saturation = 0.75 + 0.2 * sin(smoothIter * 0.1);
        float value = 0.9 + 0.1 * cos(smoothIter * 0.15);
        
        vec3 color = hsv2rgb(vec3(hue, saturation, value));
        
        // Add subtle glow effect near the set boundary
        float glowFactor = exp(-float(iterations) * 0.02);
        color = mix(color, vec3(1.0), glowFactor * 0.3);
        
        outColor = vec4(color, 1.0);
    }
}
