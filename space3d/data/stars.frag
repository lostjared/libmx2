#version 300 es
precision mediump float; 
out vec4 FragColor;
uniform vec3 starColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); 
    float alpha = smoothstep(0.5, 0.0, dist);       
    FragColor = vec4(starColor, alpha);            
}