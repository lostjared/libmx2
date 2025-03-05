#version 100
precision mediump float; 

varying vec2 vTexCoord; 
uniform sampler2D texture1; 

void main() {
    gl_FragColor = texture2D(texture1, vTexCoord);
}