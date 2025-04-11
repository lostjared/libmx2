#include"console.hpp"

namespace console {
   
    SDL_Surface* flipSurface(SDL_Surface* surface) {
        SDL_LockSurface(surface);
        Uint8* pixels = (Uint8*)surface->pixels;
        int pitch = surface->pitch;
        Uint8* tempRow = new Uint8[pitch];
        for (int y = 0; y < surface->h / 2; ++y) {
            Uint8* row1 = pixels + y * pitch;
            Uint8* row2 = pixels + (surface->h - y - 1) * pitch;
            memcpy(tempRow, row1, pitch);
            memcpy(row1, row2, pitch);
            memcpy(row2, tempRow, pitch);
        }
        
        delete[] tempRow;
        SDL_UnlockSurface(surface);
        return surface;
    }

    ConsoleChars::~ConsoleChars() {
        clear();
    }
    
    void ConsoleChars::load(const std::string &fnt, int size, const SDL_Color &col) {
        clear();
        font.loadFont(fnt, size);
        
        std::string chars = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+[]{}|;:,.<>?`~\"\'/\\-=";
        for (char c : chars) {
            SDL_Surface *surface = TTF_RenderGlyph_Solid(font.unwrap(), c, col);
            if (surface) {
                characters[c] = surface;
            } else {
                std::cerr << "Failed to load character: " << c << std::endl;
            }
        }
    }

    void ConsoleChars::clear() {
        for(auto &p : characters) {
            if(p.second) {
                SDL_FreeSurface(p.second);
            }
        }
        characters.clear();
    }

    Console::~Console() {
        if(surface) {
            SDL_FreeSurface(surface);
        }
        clear();
    }

    void Console::create(int x, int y, int w, int h) {
        console_rect.x = x;
        console_rect.y = y;
        console_rect.w = w;
        console_rect.h = h;
    
        if(surface != nullptr) {
            SDL_FreeSurface(surface);
            surface = nullptr;
        }

        surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        }
    }

    void Console::load(const std::string &fnt, int size, const SDL_Color &col) {
        c_chars.load(fnt, size, col);
        scrollToBottom();
    }
    
    void Console::clear() {
        c_chars.clear();
    }
    
    void Console::print(const std::string &str) {
        std::string currentText = data.str();
        currentText.append(str);   
        data.str("");
        data << currentText;
        cursorPos = data.str().length();
        scrollToBottom();
    }

    void Console::keypress(char c) {
        try {
            if (c == 8) { 
                if (!inputBuffer.empty()) {
                    inputBuffer.pop_back();
                }
            } else if (c == 13) { 
                std::string cmd = inputBuffer;
                inputBuffer.clear();
                procCmd(cmd);

            } else {
                inputBuffer.push_back(c);
            }
        } catch (const std::exception &e) {
            std::cerr << "Error in keypress: " << e.what() << std::endl;
        }
    }

    void Console::checkForLineWrap() {
        std::string text = data.str();
        if (cursorPos == 0 || c_chars.characters.empty()) return;
        
        int x = console_rect.x;
        int charWidth = c_chars.characters['A']->w;
        int maxWidth = console_rect.w - 50;
        
        for (size_t i = 0; i < text.length(); ++i) {
            if (i >= cursorPos) break;
            
            char c = text[i];
            if (c == '\n') {
                x = console_rect.x;
            } else {
                int width = 0;
                if (c == '\t') {
                    width = charWidth * 4;
                } else {
                    auto it = c_chars.characters.find(c);
                    if (it != c_chars.characters.end()) {
                        width = it->second->w;
                    } else {
                        width = c_chars.characters['A']->w;
                    }
                }
                
                x += width;
                
                if (x >= (console_rect.x + maxWidth)) {
                    x = console_rect.x;
                }
            }
        }
    }

    void Console::updateCursorPosition() {
        if (!surface || c_chars.characters.empty()) {
            return;
        }
        
        std::string text = data.str();
        if (text.empty()) {
            cursorX = console_rect.x;
            cursorY = console_rect.y;
            return;
        }
        
        if (cursorPos > text.length()) {
            cursorPos = text.length();
        }
        
        int x = console_rect.x;
        int y = console_rect.y;
        int maxWidth = console_rect.w - 50;
        for (size_t i = 0; i < cursorPos; ++i) {
            char c = text[i];
            
            if (c == '\n') {
                x = console_rect.x;
                y += c_chars.characters['A']->h;
                continue;
            }
            
            if (x + c_chars.characters['A']->w > console_rect.x + maxWidth) {
                x = console_rect.x;
                y += c_chars.characters['A']->h;
            }
            
            int width = 0;
            if (c == '\t') {
                width = c_chars.characters['A']->w * 4;
            } else {
                auto it = c_chars.characters.find(c);
                if (it != c_chars.characters.end()) {
                    width = it->second->w;
                } else {
                    width = c_chars.characters['A']->w;
                }
            }
            
            x += width;
        }
        
        cursorX = x;
        cursorY = y;
    }

    void Console::checkScroll() {
        if (!surface || c_chars.characters.empty()) {
            return;
        }
        
        int lineHeight = c_chars.characters['A']->h;  
        int bottomPadding = c_chars.characters['A']->h + 10; 
        int promptHeight = lineHeight + bottomPadding;
        int textAreaHeight = console_rect.h - promptHeight - 5;
        int maxLines = textAreaHeight / lineHeight;
        std::string text = data.str();
        int lineCount = 1;
        int currentLineWidth = 0;
        int charWidth = c_chars.characters['A']->w;
        int consoleWidth = console_rect.w - 50;  
        
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            if (c == '\n') {
                lineCount++;
                currentLineWidth = 0;
            } else {
                int width = 0;
                if (c == '\t') {
                    width = charWidth * 4;
                } else {
                    auto it = c_chars.characters.find(c);
                    if (it != c_chars.characters.end()) {
                        width = it->second->w;
                    } else {
                        width = c_chars.characters['A']->w;
                    }
                }
                
                currentLineWidth += width;
                if (currentLineWidth > consoleWidth) {
                    lineCount++;
                    currentLineWidth = width; 
                }
            }
        }
        
        if (lineCount > maxLines) {
            bool cursorAtEnd = (cursorPos == text.length());
            int linesToRemove = lineCount - maxLines + 1;
            size_t pos = 0;
            int linesFound = 0;
            
            while (pos < text.length() && linesFound < linesToRemove) {
                size_t nextPos = pos;
                bool foundWrap = false;
                
                
                size_t nextNewline = text.find('\n', pos);
                if (nextNewline != std::string::npos) {
                    nextPos = nextNewline + 1;
                    foundWrap = true;
                }
                
                
                if (!foundWrap) {
                    currentLineWidth = 0;
                    for (size_t i = pos; i < text.length(); ++i) {
                        char c = text[i];
                        int width = 0;
                        if (c == '\t') {
                            width = charWidth * 4;
                        } else {
                            auto it = c_chars.characters.find(c);
                            if (it != c_chars.characters.end()) {
                                width = it->second->w;
                            } else {
                                width = c_chars.characters['A']->w;
                            }
                        }
                        
                        currentLineWidth += width;
                        if (currentLineWidth > consoleWidth) {
                            nextPos = i;
                            foundWrap = true;
                            break;
                        }
                    }
                }
                
                if (foundWrap) {
                    pos = nextPos;
                    linesFound++;
                } else {
                
                    break;
                }
            }
            
            
            if (pos < text.length()) {
            
                size_t charsRemoved = pos;
                text = text.substr(pos);
                data.str(text);
            
                if (cursorAtEnd) {
                    cursorPos = text.length();
                } else {
                    if (cursorPos > charsRemoved) {
                        cursorPos -= charsRemoved;
                    } else {
                        cursorPos = 0;
                    }
                }
            
                if (stopPosition > charsRemoved) {
                    stopPosition -= charsRemoved;
                } else {
                    stopPosition = 0;
                }
            } else {
                data.str("");
                cursorPos = 0;
                stopPosition = 0;  
            }
        }
        std::string finalText = data.str();
        if (cursorPos > finalText.length()) {
            cursorPos = finalText.length();
        }
    }
    SDL_Surface *Console::drawText() {
        if (!surface) return nullptr;
        
        SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, 0, 0, 0, 188));
        int lineHeight = c_chars.characters['A']->h;
        int bottomPadding = c_chars.characters['A']->h + 10; 
        int promptY = console_rect.y + console_rect.h - lineHeight - bottomPadding;
        SDL_Rect separatorRect = {
            console_rect.x, 
            promptY - 5, 
            console_rect.w, 
            2
        };
        SDL_FillRect(surface, &separatorRect, SDL_MapRGBA(surface->format, 75, 75, 75, 220));
        std::string outputText = data.str();
        int x = console_rect.x;
        int y = console_rect.y;
        int maxWidth = console_rect.w - 50;
        
        for (size_t i = 0; i < outputText.length(); ++i) {
            if (y >= promptY - lineHeight) break;
            
            char c = outputText[i];
            if (c == '\n') {
                x = console_rect.x;
                y += lineHeight;
                continue;
            }
            
            if (x + c_chars.characters['A']->w > console_rect.x + maxWidth) {
                x = console_rect.x;
                y += lineHeight;
                if (y >= promptY - lineHeight) break;
            }
            
            auto it = c_chars.characters.find(c);
            if (it != c_chars.characters.end()) {
                SDL_Surface *char_surface = it->second;
                SDL_Rect dest_rect = {x, y, char_surface->w, char_surface->h};
                SDL_BlitSurface(char_surface, nullptr, surface, &dest_rect);
                x += char_surface->w;
            } else if (c == '\t') {
                x += c_chars.characters['A']->w * 4;
            }
        }
        
        x = console_rect.x;
        std::string promptAndInput = promptText + inputBuffer;
        for (size_t i = 0; i < promptAndInput.length(); ++i) {
            char c = promptAndInput[i];
            if (x + c_chars.characters['A']->w > console_rect.x + maxWidth) {
                x = console_rect.x;
            }
            
    
            auto it = c_chars.characters.find(c);
            if (it != c_chars.characters.end()) {
                SDL_Surface *char_surface = it->second;
                SDL_Rect dest_rect = {x, promptY, char_surface->w, char_surface->h};
                SDL_BlitSurface(char_surface, nullptr, surface, &dest_rect);
                x += char_surface->w;
            } else if (c == '\t') {
                x += c_chars.characters['A']->w * 4;
            }
        }
    
        if (cursorVisible) {
            SDL_Rect cursorRect = {x, promptY, 2, lineHeight};
            SDL_FillRect(surface, &cursorRect, SDL_MapRGBA(surface->format, 255, 255, 255, 255));
        }
        
        return surface;
    }

    std::vector<std::string> tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void Console::scrollToBottom() {
        cursorPos = data.str().length();
        updateCursorPosition();
        checkScroll();
        std::string finalText = data.str();
        if (cursorPos > finalText.length()) {
            cursorPos = finalText.length();
        }
    }

    void Console::procCmd(const std::string &cmd_text) {
        std::vector<std::string> tokens = tokenize(cmd_text);
        
        if (tokens.empty()) {
            return;
        }
        
        if (tokens[0] == "settext" && tokens.size() == 5) {
            SDL_Color col;
            int size = atoi(tokens[1].c_str());
            col.r = atoi(tokens[2].c_str());
            col.g = atoi(tokens[3].c_str());
            col.b = atoi(tokens[4].c_str());
            col.a = 255;
            if(size < 8) size = 8;
            if(size > 64) size = 64;
            
            c_chars.load(util->getFilePath("data/font.ttf"), size, col);
            
            this->print("Font updated to size " + tokens[1] + 
                        " color: " + tokens[2] + "," + tokens[3] + "," + tokens[4] + "\n");
            
        } 
        else if (tokens[0] == "about") {
            this->print("- MX2 Engine LostSideDead Software\nhttps://lostsidedead.biz\n");
        }
        else {
            this->print("- Unknown command\n");
        }
        updateCursorPosition();
        checkScroll();
    }

    GLConsole::~GLConsole() {
        if(texture) {
            glDeleteTextures(1, &texture);
        }
    }

    GLuint GLConsole::loadTextFromSurface(SDL_Surface *surf) {
        if (!surf) {
            std::cerr << "Surface is null." << std::endl;
            return 0;
        }
        GLuint texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        GLenum pixelFormat = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, 
                     pixelFormat, GL_UNSIGNED_BYTE, surf->pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return texture_id;
    }

    void GLConsole::load(gl::GLWindow *win) {
        int consoleWidth = win->w - 50;
        int consoleHeight = (win->h / 2) - 50;
        console.create(25, 25, consoleWidth, consoleHeight);
        console.load(win->util.getFilePath("data/font.ttf"), 16, {255,255,255});
        console.util = &win->util;
        console.promptText = "$ ";
        
        if(!shader.loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Error loading shader");
        }

        if(sprite == nullptr)
            sprite = std::make_unique<gl::GLSprite>();
       else
            sprite.reset(new gl::GLSprite());

        texture = loadTextFromSurface(console.drawText());
        sprite->initSize(win->w, win->h);
        sprite->initWithTexture(&shader, texture, 0.0f, 0.0f, win->w, win->h);
    }

    void GLConsole::updateTexture(GLuint tex, SDL_Surface *surf) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLConsole::draw(gl::GLWindow *win) {
        console.scrollToBottom();
        SDL_Surface *surface = console.drawText();
        if (surface) {
            shader.useProgram();
            shader.setUniform("textTexture", 0);
            flipSurface(surface);
            updateTexture(texture, surface);
            
            int consoleWidth = win->w - 50;
            int consoleHeight = (win->h / 2) - 50;
            sprite->draw(texture, 25.0f, 25.0f, consoleWidth, consoleHeight);
        } else {
            std::cerr << "Failed to draw text." << std::endl;
        }
    }

    void GLConsole::print(const std::string &data) {
        console.print(data);
    }

    void GLConsole::println(const std::string &data) {
        console.print(data + "\n");
    }

    void GLConsole::event(gl::GLWindow *win, SDL_Event &e) {
        if(e.type == SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_ESCAPE) {
                win->quit();
            } else if(e.key.keysym.sym == SDLK_RETURN) {
                console.keypress(13);
            } else if(e.key.keysym.sym == SDLK_BACKSPACE) {
                console.keypress(8);
            } else if(e.key.keysym.sym == SDLK_TAB) {
                console.keypress('\t');
            } 
        }
        else if(e.type == SDL_TEXTINPUT) {
            console.keypress(e.text.text[0]);
        }
    }

    void GLConsole::resize(gl::GLWindow *win, int w, int h) {
        load(win);
    }
}