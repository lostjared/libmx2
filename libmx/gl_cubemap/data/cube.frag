#version 300 es
precision mediump float;
out vec4 color;
in vec3 localPos; 
uniform samplerCube cubemapTexture;
uniform float time_f;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec3 uvToDir(vec2 uv) {
    float theta = uv.x * 3.14159265;         
    float phi   = uv.y * (3.14159265 * 0.5);  
    float x = cos(phi) * sin(theta);
    float y = sin(phi);
    float z = cos(phi) * cos(theta);
    return vec3(x, y, z);
}

void main(void) {

    vec3 nPos = normalize(localPos);
    float theta = atan(nPos.x, nPos.z);  
    float phi = asin(nPos.y);            
    vec2 uv = vec2(theta / 3.14159265, phi / (3.14159265 * 0.5));
    float t = mod(time_f, 10.0) * 0.1;
    float angle = sin(t * 3.14159265) * 0.5;
    float dist = length(uv);
    float bend = sin(dist * 6.0 - t * 2.0 * 3.14159265) * 0.05;
    uv = mat2(cos(angle), -sin(angle),
              sin(angle),  cos(angle)) * uv;
    
    float time_t = pingPong(time_f, 10.0);
    uv += sin(time_t * bend) + tan(bend * uv * time_t);
    
    vec3 dir = uvToDir(uv);
    dir = normalize(dir);
    
    color = texture(cubemapTexture, dir);
}
