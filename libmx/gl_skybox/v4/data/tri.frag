#version 330 core
in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube cubemapTexture;
uniform float time_f;

void main()
{    
    vec3 texCoord = normalize(TexCoords);
    FragColor = texture(cubemapTexture, texCoord);
}