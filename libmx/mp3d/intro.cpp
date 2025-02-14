#include"intro.hpp"


#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    const char *vSource = R"(#version 300 es
            precision mediump float;
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;         
            }
    )";
    /*
    const char *fSource = R"(#version 300 es
        precision mediump float;
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
        }
    )";*/

    const char *fSource = R"(#version 300 es
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoord;
    
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;
    
    void main(void) {
        vec2 uv = TexCoord * 2.0 - 1.0;
        float len = length(uv);
        float bubble = smoothstep(0.8, 1.0, 1.0 - len);
        vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
        vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
        FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
        FragColor = FragColor * alpha;
    }
    )";
    
#else
    const char *vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0); 
            TexCoord = aTexCoord;        
        }
    )";
    const char *fSource = R"(#version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform float time_f;
        uniform float alpha;
        void main(void) {
            vec2 uv = TexCoord * 2.0 - 1.0;
            float len = length(uv);
            float bubble = smoothstep(0.8, 1.0, 1.0 - len);
            vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
            vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
            FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
            FragColor = FragColor * alpha;
        }
    )";
#endif

void Intro::load_shader() {
    if(!program.loadProgramFromText(vSource, fSource)) {
        throw mx::Exception("Failed to load shader program Intro::load_shader()");
    }
}