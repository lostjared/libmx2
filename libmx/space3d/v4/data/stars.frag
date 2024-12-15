#version 330 core
out vec4 FragColor;
uniform vec3 starColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); 
    float alpha = smoothstep(0.0, 0.5, dist);      
    FragColor = vec4(starColor, alpha);            
}