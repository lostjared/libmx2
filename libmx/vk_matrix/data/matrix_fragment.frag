#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D matrixTexture;

void main() {
    vec4 texColor = texture(matrixTexture, fragTexCoord);
    
    // Output texture directly to debug - shows what texture looks like
    outColor = texColor;
}
