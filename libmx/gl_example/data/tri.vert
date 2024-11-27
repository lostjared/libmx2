#version 100
precision mediump float;

attribute vec3 position;
attribute vec3 color;

varying vec3 fragColor;

void main() {
    gl_Position = vec4(position, 1.0);
    fragColor = color;
}