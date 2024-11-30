#version 300 es
precision mediump float;

in vec2 vTexCoord;            
out vec4 fragColor;                

uniform sampler2D texture1;    
uniform float alpha;

void main() {
    vec4 color = texture(texture1, vTexCoord);
    vec4 color2 = color;
    ivec4 color_source = ivec4(color * 255.0);
    color = color * alpha;
    ivec4 colori = ivec4(color * 255.0);
    for (int i = 0; i < 3; ++i) {
        if (colori[i] >= 255)
            colori[i] = colori[i] % 255;
        
        if (color_source[i] >= 255)
            color_source[i] = color_source[i] % 255;
        
        colori[i] = colori[i] ^ color_source[i];
        color[i] = float(colori[i]) / 255.0;
    }
    
    for (int i = 0; i < 3; ++i)
        if (color[i] < 0.2) color[i] = color2[i];
        
    fragColor = color;
}
