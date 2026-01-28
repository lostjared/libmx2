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

// Pseudo-random function
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Noise function for glitch displacement
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// Convert HSV to RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Glitch color palette - vivid rainbow bands
vec3 glitchPalette(float t, float glitchAmount) {
    // Create distinct color bands
    float band = floor(t * 8.0) / 8.0;
    
    // Add time-based shifting
    float shift = pc.time * 2.0 + glitchAmount * 10.0;
    
    // Random hue jumping per frame
    float hueJump = random(vec2(floor(pc.time * 15.0), band)) * 0.3;
    
    float hue = fract(band + shift * 0.1 + hueJump);
    float sat = 0.85 + 0.15 * sin(t * 20.0 + pc.time * 5.0);
    float val = 0.9 + 0.1 * random(vec2(t, pc.time));
    
    return hsv2rgb(vec3(hue, sat, val));
}

void main() {
    vec2 resolution = ubo.params.xy;
    dvec2 fragCoord = dvec2(gl_FragCoord.xy);
    
    // Glitch displacement based on time
    float glitchTime = floor(pc.time * 20.0);
    float glitchStrength = 0.002 * (0.5 + 0.5 * sin(pc.time * 3.0));
    
    // Horizontal scanline glitch
    float scanline = floor(gl_FragCoord.y / 4.0);
    float lineGlitch = random(vec2(scanline, glitchTime)) * step(0.92, random(vec2(glitchTime, scanline)));
    
    // Apply glitch offset to coordinates
    dvec2 glitchOffset = dvec2(lineGlitch * glitchStrength * resolution.x, 0.0);
    dvec2 glitchedCoord = fragCoord + glitchOffset;
    
    // Map pixel coordinates to complex plane (using double precision)
    dvec2 uv = (glitchedCoord - 0.5 * dvec2(resolution)) / double(min(resolution.x, resolution.y));
    
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
    
    // Glitch-style coloring
    if (iterations == pc.maxIterations) {
        // Inside the set - flickering dark with occasional color bursts
        float burst = step(0.98, random(vec2(glitchTime, gl_FragCoord.x * 0.01)));
        vec3 burstColor = hsv2rgb(vec3(random(vec2(glitchTime, 0.0)), 0.9, burst));
        outColor = vec4(burstColor * 0.2, 1.0);
    } else {
        // Outside set - glitchy rainbow bands
        float zMag = float(dot(z, z));
        float smoothIter = float(iterations) - log2(log2(zMag)) + 4.0;
        
        // Create banded effect with time variation
        float bandedIter = floor(smoothIter * 1.5 + pc.time * 8.0) / 1.5;
        
        // Glitch amount based on position
        float glitchAmount = noise(vec2(float(uv) * 5.0 + pc.time));
        
        // Get glitchy palette color
        vec3 color = glitchPalette(bandedIter * 0.05, glitchAmount);
        
        // Add RGB split/chromatic aberration effect
        float rgbSplit = 0.02 * sin(pc.time * 7.0);
        vec3 colorR = glitchPalette((bandedIter + rgbSplit * 10.0) * 0.05, glitchAmount);
        vec3 colorB = glitchPalette((bandedIter - rgbSplit * 10.0) * 0.05, glitchAmount);
        
        color = vec3(colorR.r, color.g, colorB.b);
        
        // Random color channel swap per frame
        float swapChance = random(vec2(glitchTime, smoothIter * 0.1));
        if (swapChance > 0.95) {
            color = color.brg;
        } else if (swapChance > 0.90) {
            color = color.gbr;
        }
        
        // Scanline darkening
        float scanEffect = 0.95 + 0.05 * sin(gl_FragCoord.y * 2.0);
        color *= scanEffect;
        
        // Occasional white noise pixels
        float noisePixel = step(0.997, random(vec2(gl_FragCoord.xy * 0.1 + glitchTime)));
        color = mix(color, vec3(1.0), noisePixel);
        
        outColor = vec4(color, 1.0);
    }
}
