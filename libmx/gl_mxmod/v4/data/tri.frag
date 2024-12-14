#version 330 core
in vec3 fragPos;       
in vec3 fragNormal;    
in vec2 fragTexCoord;  

out vec4 fragColor;

uniform sampler2D texture1; 
uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos-fragPos);  
    vec3 viewDir = normalize(-fragPos); // In view space, the view position is (0,0,0)

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.2;
    vec3 reflectDir = reflect(-lightDir, norm); // Compute reflectDir
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = specularStrength * spec * lightColor;

    vec3 textureColor = texture(texture1, fragTexCoord).rgb; 
    vec3 result = (ambient + diffuse + specular) * textureColor;

    fragColor = vec4(result, 1.0); 
}
