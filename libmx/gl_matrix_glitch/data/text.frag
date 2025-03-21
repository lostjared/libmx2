#ifdef GL_ES
precision mediump float;
#endif

varying vec2 TexCoord;

uniform sampler2D textTexture;

void main() {
    gl_FragColor = texture2D(textTexture, TexCoord);
}
