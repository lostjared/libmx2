attribute vec3 aPos;
attribute vec2 aTexCoord;

varying vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0); // Position directly in screen space
    TexCoord = aTexCoord;         // Pass texture coordinates to the fragment shader
}
