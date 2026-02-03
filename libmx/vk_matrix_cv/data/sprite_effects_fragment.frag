#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec4 params;  // x=time, y=effectType, z=intensity, w=unused
} pc;

void main() {
    vec2 uv = fragTexCoord;
    float time = pc.params.x;
    float effectType = pc.params.y;
    float intensity = pc.params.z;
    
    vec4 texColor;
    
    if (effectType < 0.5) {
        // Effect 0: Wave distortion
        uv.x += sin(uv.y * 10.0 + time * 3.0) * 0.02 * intensity;
        uv.y += cos(uv.x * 10.0 + time * 3.0) * 0.02 * intensity;
        texColor = texture(spriteTexture, uv);
    }
    else if (effectType < 1.5) {
        // Effect 1: Color shift / hue rotation
        texColor = texture(spriteTexture, uv);
        float shift = time * 0.5 * intensity;
        mat3 hueRotation = mat3(
            0.213 + cos(shift) * 0.787 - sin(shift) * 0.213,
            0.715 - cos(shift) * 0.715 - sin(shift) * 0.715,
            0.072 - cos(shift) * 0.072 + sin(shift) * 0.928,
            0.213 - cos(shift) * 0.213 + sin(shift) * 0.143,
            0.715 + cos(shift) * 0.285 + sin(shift) * 0.140,
            0.072 - cos(shift) * 0.072 - sin(shift) * 0.283,
            0.213 - cos(shift) * 0.213 - sin(shift) * 0.787,
            0.715 - cos(shift) * 0.715 + sin(shift) * 0.715,
            0.072 + cos(shift) * 0.928 + sin(shift) * 0.072
        );
        texColor.rgb = hueRotation * texColor.rgb;
    }
    else if (effectType < 2.5) {
        // Effect 2: Pixelation
        float pixels = 64.0 / (1.0 + intensity * 10.0);
        uv = floor(uv * pixels) / pixels;
        texColor = texture(spriteTexture, uv);
    }
    else if (effectType < 3.5) {
        // Effect 3: Glow/bloom effect
        texColor = texture(spriteTexture, uv);
        vec4 blur = vec4(0.0);
        float blurSize = 0.01 * intensity;
        for (int x = -2; x <= 2; x++) {
            for (int y = -2; y <= 2; y++) {
                blur += texture(spriteTexture, uv + vec2(x, y) * blurSize);
            }
        }
        blur /= 25.0;
        texColor = mix(texColor, texColor + blur * 0.5, intensity);
    }
    else {
        // Effect 4: Pulse/breathing effect
        texColor = texture(spriteTexture, uv);
        float pulse = 0.5 + 0.5 * sin(time * 3.0);
        texColor.rgb *= 0.7 + 0.3 * pulse * intensity;
    }
    
    outColor = texColor;
}
