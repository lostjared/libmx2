#version 450
#extension GL_ARB_gpu_shader_fp64 : enable

layout(location = 0) out vec4 outColor;

// STABILITY FIX: Ensure doubles are at the top for 8-byte alignment
layout(push_constant) uniform PushConstants {
    double centerX;      // 8 bytes
    double centerY;      // 8 bytes
    double zoom;         // 8 bytes
    int maxIterations;   // 4 bytes
    float time;          // 4 bytes
} pc;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x,y = resolution
    vec4 color;
} ubo;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    // 1. STABLE RESOLUTION FETCH
    // Using ubo.params.xy; ensure these are set in your C++ code
    vec2 res = ubo.params.xy;
    if (res.x <= 0.0 || res.y <= 0.0) res = vec2(1920.0, 1080.0); // Fallback
    
    // 2. COORDINATE MAPPING (Double Precision)
    // We explicitly cast to dvec2 to ensure the subtraction and division are high-precision
    dvec2 fragCoord = dvec2(gl_FragCoord.xy);
    dvec2 uv = (fragCoord - 0.5 * dvec2(res)) / double(min(res.x, res.y));
    
    // 3. APPLY ZOOM AND PAN
    dvec2 c = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);
    dvec2 z = dvec2(0.0);
    
    // 4. ADAPTIVE ITERATIONS
    float zoomLog = float(log(max(1.0, float(pc.zoom))));
    int adaptiveMax = int(128.0 + (zoomLog * 100.0)); 
    int currentMax = clamp(adaptiveMax, 128, pc.maxIterations);
    
    int iterations = 0;
    const double ESCAPE_SQ = 1.0e8; // High escape for smoother banding

    for (int i = 0; i < currentMax; i++) {
        // TRICORN MATH: z = conj(z)^2 + c
        double next_x = z.x * z.x - z.y * z.y + c.x;
        double next_y = -2.0 * z.x * z.y + c.y; 
        
        z.x = next_x;
        z.y = next_y;
        
        if (dot(z, z) > ESCAPE_SQ) break;
        iterations++;
    }

    if (iterations == currentMax) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // 5. STABILIZED COLORING
        float zMag = float(length(z));
        // Using log2(log2()) with a large escape constant for liquid-smooth colors
        float smoothIter = float(iterations) + 1.0 - log2(log2(zMag));
        
        // Animation using pc.time
        float hue = fract(smoothIter * 0.015 + pc.time * 0.1);
        vec3 rgb = hsv2rgb(vec3(hue, 0.8, 1.0));
        
        // Darken based on iteration count to give the set "depth"
        float darkness = clamp(float(iterations) / 20.0, 0.0, 1.0);
        outColor = vec4(rgb * darkness, 1.0);
    }
}