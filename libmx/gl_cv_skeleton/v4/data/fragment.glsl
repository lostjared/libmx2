#version 330 core

in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textTexture; 
uniform float alpha;
void main() {
    vec4 fcolor = texture(textTexture, TexCoord);
    FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
}
