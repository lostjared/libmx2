#include "gameover.hpp"

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    const char *g_vSource = R"(#version 300 es
            precision highp float; 
            layout (location = 0) in vec3 aPossssss;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;         
            }
    )";
    const char *g_fSource = R"(#version 300 es
        precision highp float;   
        out vec4 color;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform vec2 iResolution;
        uniform float time_f;
        void main(void) {
            vec2 normCoord = (TexCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
            float waveStrength = 0.05;
            float waveFrequency = 3.0;
            vec2 wave = vec2(sin(normCoord.y * waveFrequency + time_f) * waveStrength,
                            cos(normCoord.x * waveFrequency + time_f) * waveStrength);

            normCoord += wave;
            float dist = length(normCoord);
            float angle = atan(normCoord.y, normCoord.x);\
            float spiralAmount = tan(time_f) * 3.0;
            angle += dist * spiralAmount;
            vec2 spiralCoord = vec2(cos(angle), sin(angle)) * dist;
            spiralCoord = (spiralCoord / vec2(iResolution.x / iResolution.y, 1.0) + 1.0) / 2.0;
            color = texture(textTexture, spiralCoord);
        }
    )";
#else
    const char *g_vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0); 
            TexCoord = aTexCoord;        
        }
    )";
    const char *g_fSource = R"(#version 330 core
        out vec4 color;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform vec2 iResolution;
        uniform float time_f;
        void main(void) {
            vec2 normCoord = (TexCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
            float waveStrength = 0.05;
            float waveFrequency = 3.0;
            vec2 wave = vec2(sin(normCoord.y * waveFrequency + time_f) * waveStrength,
                            cos(normCoord.x * waveFrequency + time_f) * waveStrength);

            normCoord += wave;
            float dist = length(normCoord);
            float angle = atan(normCoord.y, normCoord.x);\
            float spiralAmount = tan(time_f) * 3.0;
            angle += dist * spiralAmount;
            vec2 spiralCoord = vec2(cos(angle), sin(angle)) * dist;
            spiralCoord = (spiralCoord / vec2(iResolution.x / iResolution.y, 1.0) + 1.0) / 2.0;
            color = texture(textTexture, spiralCoord);
        }
    )";
#endif

void GameOver::load_shader() {
    if(!program.loadProgramFromText(g_vSource, g_fSource)) {
        throw mx::Exception("Failed to load shader program Intro::load_shader()");
    }
}