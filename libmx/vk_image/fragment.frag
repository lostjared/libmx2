#version 450

layout(location = 0) in vec3 fragColor;    
layout(location = 1) in vec2 fragTexCoord; 
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform UniformBufferObject {
    float time;      
    vec3 tintColor;  
} ubo;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = vec4(sin(texColor.rgb * (ubo.time * 2.0)), texColor.a);
}
