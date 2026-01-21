#version 330 core

in VS_OUT {
    vec2 TexCoords;
} fs_in;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler3D lutTexture;
uniform vec3 colorShift;
uniform float saturation;
uniform float brightness;
uniform float contrast;

// Convert RGB to HSV
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Convert HSV to RGB
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec4 texColor = texture(texture1, fs_in.TexCoords);
    vec3 color = texColor.rgb;
    
    // Apply brightness and contrast
    color = color * brightness;
    color = (color - 0.5) * contrast + 0.5;
    
    // Convert to HSV
    vec3 hsv = rgb2hsv(color);
    
    // Apply adjustments
    hsv.x += colorShift.x;  // Hue shift
    hsv.y *= saturation;    // Saturation
    hsv.z += colorShift.z;  // Value shift
    
    // Clamp values
    hsv.y = clamp(hsv.y, 0.0, 1.0);
    hsv.z = clamp(hsv.z, 0.0, 1.0);
    
    // Convert back to RGB
    color = hsv2rgb(hsv);
    
    FragColor = vec4(color, texColor.a);
}
