#include"intro.hpp"

#ifdef __EMSCRIPTEN__
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
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
        }
    )";
#endif

void IntroScreen::load(gl::GLWindow *win) {
    if(!program.loadProgramFromText(vSource, fSource)) {
        throw mx::Exception("Error loading shader program");
    }
    logo.initSize(win->w, win->h);
    logo.loadTexture(&program, win->util.getFilePath("data/logo.png"), 0, 0, win->w, win->h);
}


