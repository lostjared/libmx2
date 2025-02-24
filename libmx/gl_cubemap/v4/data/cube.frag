#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube cubemapTexture;
uniform bool debugMode = false;  // Add this uniform

void main()
{
    vec3 texCoords = normalize(localPos);
    
    if (debugMode) {
        // Debug visualization
        vec3 absCoords = abs(texCoords);
        if(absCoords.x >= absCoords.y && absCoords.x >= absCoords.z)
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        else if(absCoords.y >= absCoords.x && absCoords.y >= absCoords.z)
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        else
            FragColor = vec4(0.0, 0.0, 1.0, 1.0);
    } else {
        // Normal cubemap rendering
        FragColor = texture(cubemapTexture, texCoords);
    }
}