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
    vec4 params; // x,y = resolution
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
    
    // Map pixel coordinates to complex plane with aspect ratio correction
    dvec2 uv = (fragCoord - 0.5 * dvec2(resolution)) / double(min(resolution.x, resolution.y));
    
    // Apply zoom and center offset
    dvec2 c = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);
    
    dvec2 z = dvec2(0.0);
    int iterations = 0;
    const double ESCAPE = 256.0; 
    
    for (int i = 0; i < pc.maxIterations; i++) {
        // --- TRICORN LOGIC ---
        // Step 1: Complex Conjugate (negate the imaginary part)
        z.y = -z.y;
        
        // Step 2: Escape Check
        if (dot(z, z) > ESCAPE * ESCAPE) break;
        
        // Step 3: Square and add C (Standard z = z^2 + c)
        // (x + yi)^2 = x^2 - y^2 + 2xyi
        z = dvec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        
        iterations++;
    }
    
    if (iterations == pc.maxIterations) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Smooth Iteration Calculation
        float zMag = float(length(z));
        float smoothIter = float(iterations) + 1.0 - log2(log2(zMag)) / log2(2.0);
        
        // Palette: I've shifted this toward "Electric Blues/Purples" 
        // which look great on the Tricorn's jagged edges.
        float hue = (smoothIter * 0.02) + (pc.time * 0.1);
        vec3 color = hsv2rgb(vec3(fract(0.6 + hue * 0.5), 0.7, 1.0));
        
        // Simple ambient occlusion-style darkening for depth
        color *= smoothstep(0.0, 1.0, float(iterations) / 20.0);

        outColor = vec4(color, 1.0);
    }
}