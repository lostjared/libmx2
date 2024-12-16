#version 300 es
precision highp float;

in vec3 fragPos;       
in vec3 fragNormal;    
in vec2 fragTexCoord;  
out vec4 fragColor;

uniform sampler2D texture1; 
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = specularStrength * spec * lightColor;
    vec3 textureColor = texture(texture1, fragTexCoord).rgb; 
    vec3 result = (ambient + diffuse + specular) * textureColor;
    fragColor = vec4(result, 1.0); 
}
