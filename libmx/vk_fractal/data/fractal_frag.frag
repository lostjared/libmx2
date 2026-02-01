#version 450
#extension GL_ARB_gpu_shader_fp64 : enable

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    double centerX;
    double centerY;
    double zoom;
    int maxIterations; // This now acts as our "Limit" or "Cap"
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
    // Use double precision throughout coordinate calculation
    double resX = double(ubo.params.x);
    double resY = double(ubo.params.y);
    double minRes = min(resX, resY);
    
    // gl_FragCoord is float, but pixel coordinates are integers so conversion is exact
    double fragX = double(gl_FragCoord.x);
    double fragY = double(gl_FragCoord.y);
    
    // Center the coordinates and normalize
    double uvX = (fragX - resX * 0.5LF) / minRes;
    double uvY = (fragY - resY * 0.5LF) / minRes;
    
    // Apply zoom and pan - all in double precision
    double cx = uvX / pc.zoom + pc.centerX;
    double cy = uvY / pc.zoom + pc.centerY;
    
    double zx = 0.0LF;
    double zy = 0.0LF;
    
    // --- ADAPTIVE ITERATION LOGIC ---
    // Higher zoom = more iterations needed. 
    // We use log10 of the zoom to scale the iterations linearly with depth.
    float zoomLevel = float(log(float(pc.zoom)) / log(10.0));
    int adaptiveMax = int(128.0 + (zoomLevel * 150.0)); 
    
    // Clamp the value between a reasonable floor and the push constant limit
    int currentMax = clamp(adaptiveMax, 128, pc.maxIterations);
    
    int iterations = 0;
    const double ESCAPE_RADIUS_SQ = 4.0LF; // Standard escape radius for tricorn

    for (int i = 0; i < currentMax; i++) {
        // Tricorn fractal: z = conj(z)^2 + c
        // conj(z) = (zx, -zy)
        // conj(z)^2 = zx^2 - zy^2 - 2i*zx*zy
        double zx_new = zx * zx - zy * zy + cx;
        double zy_new = -2.0LF * zx * zy + cy;
        
        zx = zx_new;
        zy = zy_new;
        
        // Check escape AFTER computing new z
        double magSq = zx * zx + zy * zy;
        if (magSq > ESCAPE_RADIUS_SQ) break;
        iterations++;
    }

    if (iterations == currentMax) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Use double precision for magnitude calculation
        double zMagSq = zx * zx + zy * zy;
        float zMag = float(sqrt(zMagSq));
        // Smooth iteration count for tricorn (power 2 fractal)
        float smoothIter = float(iterations) + 1.0 - log2(log2(max(zMag, 1.0)));
        
        // Speed and color cycling using pc.time
        float hue = fract(smoothIter * 0.01 + pc.time * 0.1);
        vec3 color = hsv2rgb(vec3(hue, 0.75, 1.0));
        
        // Anti-pixelation: fade out colors if we are near the max iterations
        float edgeFade = 1.0 - (float(iterations) / float(currentMax));
        color *= edgeFade;

        outColor = vec4(color, 1.0);
    }
}