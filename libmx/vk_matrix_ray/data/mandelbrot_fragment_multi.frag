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
    vec4 params;      // x=width, y=height, z=fractal_type, w=color_scheme
    vec4 color;       // x=julia_c.x, y=julia_c.y, z=unused, w=unused
} ubo;

float rand(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec3 getColor(int n, int scheme, float time) {
    if (n == pc.maxIterations) {
        return vec3(0.0, 0.0, 0.0);
    }
    
    float t = float(n) / float(pc.maxIterations);
    
    if (scheme == 0) { // Random
        float r = rand(float(n) + time);
        float g = rand(float(n) + time + 1.0);
        float b = rand(float(n) + time + 2.0);
        return vec3(r, g, b);
    } else if (scheme == 1) { // Fire
        return vec3(t, t*t, t*t*t);
    } else if (scheme == 2) { // Ocean
        return vec3(0.0, t*0.5, t);
    } else if (scheme == 3) { // Nebula
        return vec3(t*0.8 + 0.2, t*0.3, t*0.9);
    } else if (scheme == 4) { // Grayscale
        return vec3(t, t, t);
    } else { // Psychedelic (scheme == 5)
        return vec3(
            sin(t * 6.28 + time) * 0.5 + 0.5,
            sin(t * 6.28 + time + 2.09) * 0.5 + 0.5,
            sin(t * 6.28 + time + 4.18) * 0.5 + 0.5
        );
    }
}

int computeFractal(dvec2 c, int fractalType, dvec2 juliaC) {
    dvec2 z = dvec2(0.0);
    
    if (fractalType == 1) { // Julia
        z = c;
        c = juliaC;
    }
    
    int n = 0;
    for (int i = 0; i < pc.maxIterations; i++) {
        if (fractalType == 2) { // Burning Ship
            z = dvec2(
                abs(z.x) * abs(z.x) - abs(z.y) * abs(z.y) + c.x,
                2.0 * abs(z.x) * abs(z.y) + c.y
            );
        } else if (fractalType == 3) { // Tricorn
            z = dvec2(
                z.x * z.x - z.y * z.y + c.x,
                -2.0 * z.x * z.y + c.y
            );
        } else { // Mandelbrot (0) or Julia (1)
            z = dvec2(
                z.x * z.x - z.y * z.y + c.x,
                2.0 * z.x * z.y + c.y
            );
        }
        
        if (dot(z, z) > 4.0) break;
        n++;
    }
    return n;
}

void main() {
    vec2 resolution = ubo.params.xy;
    int fractalType = int(ubo.params.z);
    int colorScheme = int(ubo.params.w);
    dvec2 juliaC = dvec2(ubo.color.x, ubo.color.y); 
    dvec2 fragCoord = dvec2(gl_FragCoord.xy);
    dvec2 uv = (fragCoord - 0.5 * dvec2(resolution)) / double(min(resolution.x, resolution.y));
    dvec2 c = uv / pc.zoom + dvec2(pc.centerX, pc.centerY);
    int n = computeFractal(c, fractalType, juliaC);
    vec3 color = getColor(n, colorScheme, pc.time);
    outColor = vec4(color, 1.0);
}
