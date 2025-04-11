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
    
    void ConsoleChars::load(const std::string &fnt, int size) {

        font.loadFont(fnt, size);
        std::string chars = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+[]{}|;:,.<>?`~/\\-=";
        for (char c : chars) {
            SDL_Surface *surface = TTF_RenderGlyph_Solid(font.unwrap(), c, {255, 255, 255});
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
        } else {
            // Initialize with transparent black background
            SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 188));
        }
    }

    void Console::load(const std::string &fnt, int size) {
        c_chars.load(fnt, size);
    }
    
    void Console::clear() {
        c_chars.clear();
    }
    
    void Console::print(const std::string &str) {
        
        std::string text = data.str();
        
        
        text.insert(cursorPos, str);
        data.str(text);
        
        
        cursorPos += str.length();
        
        
        updateCursorPosition();
        checkScroll();
    }

    void Console::keypress(char c) {
        std::string text = data.str();
        
        if (c == 8) { 
            if (cursorPos > 0) {
                text.erase(cursorPos - 1, 1);
                data.str(text);
                cursorPos--;
            }
        } else if (c == 13) { 
            text.insert(cursorPos, 1, '\n');
            data.str(text);
            cursorPos++;
        } else {
            text.insert(cursorPos, 1, c);
            data.str(text);
            cursorPos++;
            
            
            checkForLineWrap();
        }
        
        
        checkScroll();
        updateCursorPosition();
    }

    void Console::checkForLineWrap() {
        std::string text = data.str();
        if (cursorPos == 0 || c_chars.characters.empty()) return;
        
        int x = console_rect.x;
        int maxWidth = console_rect.w - 50;
        
        for (size_t i = 0; i < text.length(); ++i) {
            if (i >= cursorPos) break;
            
            char c = text[i];
            if (c == '\n') {
                x = console_rect.x;
            } else {
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
        int maxLines = (console_rect.h / lineHeight) - 1;  
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
                        width = charWidth;
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
                                width = charWidth;
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
            } else {
                data.str("");
                cursorPos = 0;
            }
        }
    }

    SDL_Surface *Console::drawText() {
        if (!surface) {
            std::cerr << "Surface not created." << std::endl;
            return nullptr;
        }

        SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, 0, 0, 0, 188));
        std::string text = data.str();
        int x = console_rect.x;
        int y = console_rect.y;
        int maxWidth = console_rect.w - 50;
        
        for (size_t i = 0; i < text.length(); ++i) {
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
            
            auto it = c_chars.characters.find(c);
            if (it != c_chars.characters.end()) {
                SDL_Surface *char_surface = it->second;
                SDL_Rect dest_rect = {x, y, char_surface->w, char_surface->h};
                SDL_BlitSurface(char_surface, nullptr, surface, &dest_rect);
                x += char_surface->w;
            } else if (c == '\t') {
                x += c_chars.characters['A']->w * 4;
            }
            
            if (y + c_chars.characters['A']->h > surface->h) {
                break;
            }
        }
        
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - cursorBlinkTime > 500) {
            cursorVisible = !cursorVisible;
            cursorBlinkTime = currentTime;
        }
        
        if (cursorVisible) {
            if (cursorY + c_chars.characters['A']->h <= console_rect.y + console_rect.h) {
                SDL_Rect cursorRect = {cursorX, cursorY, 2, c_chars.characters['A']->h};
                SDL_FillRect(surface, &cursorRect, SDL_MapRGBA(surface->format, 255, 255, 255, 255));
            }
        }
        
        return surface;
    }

    std::string Console::getText() const {
        return data.str();
    }

    void Console::setText(const std::string &text) {
        data.str(text);
        updateCursorPosition();
    }

    size_t Console::getCursorPos() const {
        return cursorPos;
    }

    void Console::setCursorPos(size_t pos) {
        cursorPos = pos;
        updateCursorPosition();
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
        
        console.load(win->util.getFilePath("data/font.ttf"), 16);
        
        if(!shader.loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Error loading shader");
        }

        if(sprite == nullptr)
            sprite = std::make_unique<gl::GLSprite>();
        else {
            sprite.reset(new gl::GLSprite());
        }

        texture = loadTextFromSurface(console.drawText());
        sprite->initSize(win->w, win->h);
        sprite->initWithTexture(&shader, texture, 0.0f, 0.0f, win->w, win->h);
    }

    void GLConsole::updateTexture(GLuint tex, SDL_Surface *surf) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
    }

    void GLConsole::draw(gl::GLWindow *win) {
        SDL_Surface *surface = console.drawText();
        if (surface) {
        
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            shader.useProgram();
            shader.setUniform("textTexture", 0);
            
            flipSurface(surface);
            glActiveTexture(GL_TEXTURE0);
            sprite->updateTexture(surface);
        
            int consoleWidth = win->w - 50;
            int consoleHeight = (win->h / 2) - 50;
            sprite->draw(texture, 25.0f, 25.0f, consoleWidth, consoleHeight);
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glDisable(GL_BLEND);
    }

    void GLConsole::print(const std::string &data) {
        console.print(data);
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