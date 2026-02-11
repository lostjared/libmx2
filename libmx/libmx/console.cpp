#include"console.hpp"
#ifdef WITH_GL
#include"gl.hpp"
#endif

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

    ConsoleChars::ConsoleChars() {
        font = std::make_unique<mx::Font>();
        if (!font) {
            std::cerr << "Failed to create font object" << std::endl;
        }
    }

    ConsoleChars::~ConsoleChars() {
        clear();
    }
    
    void ConsoleChars::load(const std::string &fnt, int size, const SDL_Color &col) {
        clear();
        font.reset(new mx::Font(fnt, size));
        if(font) {
            std::string chars = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+[]{}|;:,.<>?`~\"\'/\\-=";
            for (char c : chars) {
                SDL_Surface *surface = TTF_RenderGlyph_Solid(font->unwrap(), c, col);
                if (surface) {
                    characters[c] = surface;
                } else {
                    std::cerr << "Failed to load character: " << c << std::endl;
                }
            }
        }
    }

    void ConsoleChars::clear() {
        if(!characters.empty()) {
            for(auto &p : characters) {
                if(p.second != nullptr) {
                    SDL_FreeSurface(p.second);
                    p.second = nullptr;
                }
            }
            characters.clear();
            font.reset();
        }
    }

    void CommandQueue::push(const Command& cmd) {
#if THREAD_ENABLED
        std::lock_guard<std::mutex> lock(queue_mutex);
        queue.push(cmd);
        cv.notify_one();
#else
        queue.push(cmd);
#endif
    }

    bool CommandQueue::pop(Command& cmd) {
#if THREAD_ENABLED
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait_for(lock, std::chrono::milliseconds(100), 
            [this] { return !queue.empty() || !active; });
        
        if (!active && queue.empty())
            return false;
#else
        if (queue.empty())
            return false;
#endif
        
        if (!queue.empty()) {
            cmd = queue.front();
            queue.pop();
            return true;
        }
        return false;
    }

    void CommandQueue::shutdown() {
#if THREAD_ENABLED
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            active = false;
        }
        cv.notify_all();
#else
        active = false;
#endif
    }

    Console::Console() { 
        inMultilineMode = false;
        braceCount = 0;
        multilineBuffer = "";
        originalPrompt = "$ ";
        worker_active = true;
        
#if THREAD_ENABLED
        worker_thread = std::make_unique<std::thread>(&Console::worker_thread_func, this);
#endif
    }

    Console::~Console() {
        worker_active = false;
        command_queue.shutdown();
        
#if THREAD_ENABLED
        if (worker_thread && worker_thread->joinable()) {
            worker_thread->join();
        }
#endif
        
        if(surface) {
            SDL_FreeSurface(surface);
            surface = nullptr;
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

    void Console::reload() {
        c_chars.load(font, font_size, color);
    }

    void Console::load(const std::string &fnt, int size, const SDL_Color &col) {
        font = fnt;
        font_size = size;
        color = col;
        c_chars.load(fnt, size, col);
        scrollToBottom();
    }
    
    void Console::clear() {
        c_chars.clear();
    }
    
    void Console::scrollUp() {
        THREAD_GUARD(console_mutex);
        userScrolling = true;
        int maxOffset = std::max(0, (int)lines.size() - visibleLineCount);
        if (scrollOffset < maxOffset) {
            ++scrollOffset;
            needsRedraw = true;
           
        }
    }

    void Console::scrollDown() {
        THREAD_GUARD(console_mutex);
        userScrolling = true;
        if (scrollOffset > 0) {
            --scrollOffset;
            needsRedraw = true;
        }
    }

    void Console::resetScroll() {
        if (scrollOffset != 0) {
            scrollOffset = 0;
            needsRedraw = true;
            userScrolling = false; 
        }
    }

 
 
    void Console::print(const std::string &str) {
        THREAD_GUARD(console_mutex);
        data << str; 
        cursorPos = data.str().length();
        needsReflow = true;
        needsRedraw = true;
        scrollToBottom();
    }
 
    void Console::setCallback(gl::GLWindow *window, std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback) {
        this->callback = callback;
        callbackSet = true;
    }

    void Console::updateInputScrollOffset() {
        if (c_chars.characters.empty()) return;
        
        int maxWidth = console_rect.w - 50;
        int promptWidth = 0;
        
        
        for (char c : promptText) {
            int charWidth = 0;
            if (c == '\t') {
                charWidth = c_chars.characters['A']->w * 4;
            } else {
                auto it = c_chars.characters.find(c);
                charWidth = (it != c_chars.characters.end()) ? 
                            it->second->w : c_chars.characters['A']->w;
            }
            promptWidth += charWidth;
        }
        
        
        int cursorPixelPos = promptWidth;
        for (int i = inputScrollOffset; i < static_cast<int>(inputCursorPos); ++i) {
            if (i >= 0 && i < (int)inputBuffer.size()) {
                char c = inputBuffer[i];
                int charWidth = 0;
                if (c == '\t') {
                    charWidth = c_chars.characters['A']->w * 4;
                } else {
                    auto it = c_chars.characters.find(c);
                    charWidth = (it != c_chars.characters.end()) ? 
                                it->second->w : c_chars.characters['A']->w;
                }
                cursorPixelPos += charWidth;
            }
        }
        
        
        while (cursorPixelPos > maxWidth && inputScrollOffset < (int)inputBuffer.size()) {
            if (inputScrollOffset < (int)inputBuffer.size()) {
                char c = inputBuffer[inputScrollOffset];
                int charWidth = 0;
                if (c == '\t') {
                    charWidth = c_chars.characters['A']->w * 4;
                } else {
                    auto it = c_chars.characters.find(c);
                    charWidth = (it != c_chars.characters.end()) ? 
                                it->second->w : c_chars.characters['A']->w;
                }
                cursorPixelPos -= charWidth;
                inputScrollOffset++;
            } else {
                break;
            }
        }
        
        if (static_cast<int>(inputCursorPos) < inputScrollOffset) {
            inputScrollOffset = inputCursorPos;
        }
    }

    void Console::keypress(char c) {
        THREAD_GUARD(console_mutex);
        needsRedraw = true;
        try {
            if (c == 8) { 
                if (!inputBuffer.empty() && inputCursorPos > 0 && inputCursorPos <= inputBuffer.size()) {
                    inputBuffer.erase(inputCursorPos - 1, 1);
                    inputCursorPos--;
                    if (inputScrollOffset > 0) {
                        inputScrollOffset--;
                    }
                    
                    if (static_cast<int>(inputCursorPos) < inputScrollOffset) {
                        inputScrollOffset = inputCursorPos;
                    }
                    
                    updateInputScrollOffset();
                }
            } else if (c == 127) { 
                if (!inputBuffer.empty() && inputCursorPos < inputBuffer.size()) {
                    inputBuffer.erase(inputCursorPos, 1);
                    
                    if (inputScrollOffset > static_cast<int>(inputBuffer.size())) {
                        inputScrollOffset = std::max(0, static_cast<int>(inputBuffer.size()));
                    }
                    
                    updateInputScrollOffset();
                }
            } else if (c == 13) { 
                
                inputScrollOffset = 0;
                
                std::string cmd = inputBuffer;
                
                if (inMultilineMode) {
                    multilineBuffer += cmd + "\n";
                    inputBuffer.clear();
                    inputCursorPos = 0;
                    
                    for (char ch : cmd) {
                        if (ch == '{') braceCount++;
                        else if (ch == '}') braceCount--;
                    }
                    
                    if (braceCount == 0) {
                        inMultilineMode = false;
                        promptText = originalPrompt;
                        
                        std::string processedCommand = multilineBuffer;
                        size_t firstOpenBrace = processedCommand.find('{');
                        if (firstOpenBrace != std::string::npos) {
                            processedCommand.erase(firstOpenBrace, 1);
                        }
                        size_t lastCloseBrace = processedCommand.find_last_of('}');
                        if (lastCloseBrace != std::string::npos) {
                            processedCommand.erase(lastCloseBrace, 1);
                        }
                        inputScrollOffset = 0;    
                        procCmd(processedCommand);
                        multilineBuffer.clear();
                        scrollOffset = 0;
                        userScrolling = false;
                        scrollToBottom();
                        this->process_message_queue();
                    } else {
                        this->thread_safe_print(cmd + "\n");
                        needsRedraw = true;
                        needsReflow = true;
                        scrollOffset = 0;
                        userScrolling = false;
                        scrollToBottom();
                        this->process_message_queue();
                    }
                } else {
                    inputBuffer.clear();
                    inputCursorPos = 0;
                    
                    if (cmd.find('{') != std::string::npos && cmd.find('}') == std::string::npos) {
                        braceCount = 0;  
                        for (char ch : cmd) {
                            if (ch == '{') braceCount++;
                            else if (ch == '}') braceCount--;
                        }
                        
                        if (braceCount > 0) {
                            inMultilineMode = true;
                            originalPrompt = promptText;
                            promptText = "... ";
                            multilineBuffer = cmd + "\n";
                            this->thread_safe_print(cmd + "\n");
                            return;
                        }
                    }    
                    procCmd(cmd);
                }
                
                scrollToBottom();
            } else if (c == '{') {
                inputBuffer.insert(inputCursorPos, 1, c);
                inputCursorPos++;
                updateInputScrollOffset();
                
                if (!inMultilineMode) {
                    braceCount++;  
                }
            } else if (c == '}') {
                inputBuffer.insert(inputCursorPos, 1, c);
                inputCursorPos++;
                updateInputScrollOffset();
                
                if (inMultilineMode) {
                    braceCount--;
                    if (braceCount == 0) {
                        std::string processedCommand;
                        size_t firstOpenBrace = multilineBuffer.find('{');
                        if (firstOpenBrace != std::string::npos) {
                            multilineBuffer.erase(firstOpenBrace, 1);
                        }
                        size_t lastCloseBrace = inputBuffer.find_last_of('}');
                        if (lastCloseBrace != std::string::npos) {
                            inputBuffer.erase(lastCloseBrace, 1);
                        }
                        processedCommand = multilineBuffer + inputBuffer;
                        inMultilineMode = false;
                        promptText = originalPrompt;
                        this->thread_safe_print(inputBuffer + "\n");
                        inputBuffer.clear();
                        inputCursorPos = 0;
                        multilineBuffer.clear();
                        procCmd(processedCommand + "\n");                          
                        needsRedraw = true;
                        scrollToBottom();
                        this->process_message_queue();
                    }
                }
            } else {
                inputBuffer.insert(inputCursorPos, 1, c);
                inputCursorPos++;
                updateInputScrollOffset();
            }
        } catch (const std::exception &e) {
            std::cerr << "Error in keypress: " << e.what() << std::endl;
        }
    }

    void Console::checkForLineWrap() {
        THREAD_GUARD(console_mutex);
        std::string text = data.str();
        if (cursorPos == 0 || c_chars.characters.empty()) return;
        
        int x = console_rect.x;
        int y = console_rect.y; 
        int charWidth = c_chars.characters['A']->w;
        int maxWidth = console_rect.w - 50;
        
        for (size_t i = 0; i < text.length(); ++i) {
            if (i >= cursorPos) break;
            
            char c = text[i];
            if (c == '\n') {
                x = console_rect.x;
                y += c_chars.characters['A']->h; 
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
                    y += c_chars.characters['A']->h;
                }
            }
        }
    }

    void Console::updateCursorPosition() {
        THREAD_GUARD(console_mutex);
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
        THREAD_GUARD(console_mutex);
        if (!surface || c_chars.characters.empty() || c_chars.characters.find('A') == c_chars.characters.end()) {
            return;
        }
        if (needsReflow) {
            calculateLines();
        }
        int charHeight = c_chars.characters['A']->h;
        int bottomPadding = charHeight + 10;
        int promptY = console_rect.y + console_rect.h - charHeight - bottomPadding;
        int maxVisibleLines = (promptY - console_rect.y) / charHeight;
        int maxScrollbackLines = maxVisibleLines * 50; 
        
        while (static_cast<int>(lines.size()) > maxScrollbackLines && !lines.empty()) {
            size_t removeLen = lines[0].length;
            std::string buffer = data.str();
            if (removeLen < buffer.length() && buffer[removeLen] == '\n') {
                removeLen++;
            }
            if (removeLen > buffer.length()) removeLen = buffer.length();
            std::string newBuffer = buffer.substr(removeLen);
            data.str("");
            data << newBuffer;
            lines.clear();
            calculateLines();
        }
        std::string finalText = data.str();
        if (cursorPos > finalText.length()) {
            cursorPos = finalText.length();
        }
    }

    void Console::calculateLines() {
        THREAD_GUARD(console_mutex);
        
        auto it_A_check = c_chars.characters.find('A'); 
        if (!surface || c_chars.characters.empty() || it_A_check == c_chars.characters.end() || !it_A_check->second || it_A_check->second->h == 0) {
            return;
        }
        
        lines.clear(); 
        std::string text = data.str();
        int maxWidth = console_rect.w - 50; 
        
        size_t lineStart = 0;
        size_t currentPos = 0;
        int currentLineWidth = 0;
        
        SDL_Surface* char_A_surface = it_A_check->second;
        int defaultCharWidth = char_A_surface->w;
        int lineHeight = char_A_surface->h; 

        while (currentPos < text.length()) {
            char c = text[currentPos];
            int charWidth = 0;

            if (c == '\n') {
                lines.push_back({text.substr(lineStart, currentPos - lineStart), lineStart, currentPos - lineStart});
                lineStart = currentPos + 1;
                currentLineWidth = 0;
                currentPos++;
                continue;
            }

            if (c == '\t') {
                charWidth = defaultCharWidth * 4; 
            } else {
                auto it_char = c_chars.characters.find(c);
                if (it_char != c_chars.characters.end() && it_char->second) {
                    charWidth = it_char->second->w;
                } else {
                    charWidth = defaultCharWidth; 
                }
            }

            if (lineStart < currentPos && (currentLineWidth + charWidth) > maxWidth) {
                lines.push_back({text.substr(lineStart, currentPos - lineStart), lineStart, currentPos - lineStart});
                lineStart = currentPos; 
                currentLineWidth = charWidth;
            } else {
                currentLineWidth += charWidth;
            }
            currentPos++;
        }

        if (lineStart < text.length()) {
            lines.push_back({text.substr(lineStart), lineStart, text.length() - lineStart});
        }
        
        needsReflow = false;

        
        int bottomPadding = lineHeight + 10;
        int promptY = console_rect.y + console_rect.h - lineHeight - bottomPadding;
        
        
        int maxVisibleLines = (promptY - console_rect.y) / lineHeight;
        maxVisibleLines -= 1; 
        if (maxVisibleLines < 1) maxVisibleLines = 1;
        
        
        visibleLineCount = maxVisibleLines;
    }

    SDL_Surface *Console::drawText() {
         THREAD_GUARD(console_mutex);
        if (fadeState != FADE_NONE) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsedTime = currentTime - fadeStartTime;

            if (fadeState == FADE_IN) {
                if (elapsedTime >= fadeDuration) {
                    alpha = 188;
                    fadeState = FADE_NONE;
                } else {
                    float t = (float)elapsedTime / (float)fadeDuration;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    alpha = (unsigned char)(1 + (188 - 1) * t);
                }
            } else if (fadeState == FADE_OUT) {
                if (elapsedTime >= fadeDuration) {
                    alpha = 0;
                    fadeState = FADE_NONE;
                } else {
                    float t = (float)elapsedTime / (float)fadeDuration;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    alpha = (unsigned char)(188 * (1.0f - t));
                }
            } 
            needsRedraw = true;
        }
    
        if (!surface) return nullptr;
        if(!needsRedraw && fadeState == FADE_NONE && surface) return surface;

        SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, 0, 0, 0, alpha));

        if (alpha < 5) {
            return surface;
        }
        
        if (needsReflow) {
            calculateLines();
        }
        
        int lineHeight = c_chars.characters['A']->h;
        int bottomPadding = lineHeight + 10;
        int promptY = console_rect.y + console_rect.h - lineHeight - bottomPadding;
        
        SDL_Rect separatorRect = {
            console_rect.x, 
            promptY - 5, 
            console_rect.w, 
            2
        };
        SDL_FillRect(surface, &separatorRect, SDL_MapRGBA(surface->format, 75, 75, 75, 220));
        
        int maxVisibleLines = (promptY - console_rect.y) / lineHeight;
        maxVisibleLines -= 1; 
        if (maxVisibleLines < 1) maxVisibleLines = 1;  
    
        size_t startLine = 0;
        if (static_cast<int>(lines.size()) > maxVisibleLines) {
            int totalLines = lines.size();
            int lastVisibleLine = totalLines - 1 - scrollOffset; 
            startLine = std::max(0, lastVisibleLine - maxVisibleLines + 1);
        } else {
            startLine = 0;
        }

        int y = console_rect.y;
        for (size_t i = startLine; i < lines.size() && i < startLine + maxVisibleLines; ++i) {
            const auto& line = lines[i];
            if (y >= promptY) break; 
            
            int x = console_rect.x;
            for (char c : line.text) {
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
            
            y += lineHeight;
        }
        
        
        int x = console_rect.x;
        if (inputCursorPos > inputBuffer.size())
            inputCursorPos = inputBuffer.size();
        
        for (char c : promptText) {
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
        
        int cursorXPos = x;
        int maxWidth = console_rect.w - 50;

        for (int i = inputScrollOffset; i < (int)inputBuffer.size(); ++i) {
            char c = inputBuffer[i];
            
            if (i == static_cast<int>(inputCursorPos)) {
                cursorXPos = x;
            }
            
            int charWidth = 0;
            if (c == '\t') {
                charWidth = c_chars.characters['A']->w * 4;
            } else {
                auto it = c_chars.characters.find(c);
                charWidth = (it != c_chars.characters.end()) ? 
                            it->second->w : c_chars.characters['A']->w;
            }
            
            if (x + charWidth > console_rect.x + maxWidth) {
                break;
            }
        
            if (c != '\t') {
                auto it = c_chars.characters.find(c);
                if (it != c_chars.characters.end()) {
                    SDL_Surface *char_surface = it->second;
                    SDL_Rect dest_rect = {x, promptY, char_surface->w, char_surface->h};
                    SDL_BlitSurface(char_surface, nullptr, surface, &dest_rect);
                }
            }
            
            x += charWidth;
        }
        
        if (inputCursorPos >= inputBuffer.size()) {
            cursorXPos = x;
        }
        
        if (cursorVisible) {
            SDL_Rect cursorRect = {cursorXPos, promptY, 2, c_chars.characters['A']->h};
            SDL_FillRect(surface, &cursorRect, SDL_MapRGBA(surface->format, 255, 255, 255, 255));
        }
        needsRedraw = false;
        
        int total = lines.size();
        if (total > visibleLineCount) {
            int scrollbarWidth = 15;
            int scrollbarX = console_rect.w - scrollbarWidth - 5; 
            int scrollbarHeight = console_rect.h - 80; 
            int scrollbarY = 10; 
            
            SDL_Rect trackRect = {scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight};
            SDL_FillRect(surface, &trackRect, SDL_MapRGBA(surface->format, 40, 40, 40, 180));
            
            scrollbarRect.x = console_rect.x + trackRect.x;
            scrollbarRect.y = console_rect.y + trackRect.y;
            scrollbarRect.w = trackRect.w;
            scrollbarRect.h = trackRect.h;

            float thumbRatio = (float)visibleLineCount / total;
            int thumbHeight = std::max(20, (int)(thumbRatio * scrollbarHeight));
            float scrollProgress = 1.0f - ((float)scrollOffset / (total - visibleLineCount));
            int thumbY = scrollbarY + (int)(scrollProgress * (scrollbarHeight - thumbHeight));
            
            SDL_Rect thumbRect = {scrollbarX + 1, thumbY, scrollbarWidth - 2, thumbHeight};
            SDL_FillRect(surface, &thumbRect, SDL_MapRGBA(surface->format, 120, 120, 120, 0));  
        } 
        flipSurface(surface);
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
        THREAD_GUARD(console_mutex);
        scrollOffset = 0;        
        userScrolling = false;   
        cursorPos = data.str().length();
        updateCursorPosition();
        checkScroll();  
        inputCursorPos = inputBuffer.length();
        needsRedraw = true;      
    }

    void Console::moveCursorLeft() {
        THREAD_GUARD(console_mutex);
        if (inputCursorPos > 0) {
            inputCursorPos--;
            
            if (static_cast<int>(inputCursorPos) < inputScrollOffset) {
                inputScrollOffset = inputCursorPos;
                needsRedraw = true;
            }
            
            updateInputScrollOffset();
        }
    }

    void Console::moveCursorRight() {
        THREAD_GUARD(console_mutex);
        if (inputCursorPos < inputBuffer.length()) {
            inputCursorPos++;
            updateInputScrollOffset();
        }
    }

    void Console::moveHistoryUp() {
        THREAD_GUARD(console_mutex);
        if (historyIndex == -1 && !inputBuffer.empty()) {
            tempBuffer = inputBuffer;
        }
        if (!commandHistory.empty() && (historyIndex < static_cast<int>(commandHistory.size()) - 1)) {
            historyIndex++;
            inputBuffer = commandHistory[commandHistory.size() - 1 - historyIndex];
            inputCursorPos = inputBuffer.length();
            inputScrollOffset = 0; 
            updateInputScrollOffset();
        }
    }

    void Console::moveHistoryDown() {
        THREAD_GUARD(console_mutex);
        if (historyIndex > 0) {
            historyIndex--;
            inputBuffer = commandHistory[commandHistory.size() - 1 - historyIndex];
            inputCursorPos = inputBuffer.length();
            inputScrollOffset = 0;  
            updateInputScrollOffset();
        }
        else if (historyIndex == 0) {
            historyIndex = -1;
            inputBuffer = tempBuffer;
            inputCursorPos = inputBuffer.length();
            inputScrollOffset = 0;  
            updateInputScrollOffset();
        }
    }

    std::istream &GLConsole::inputData() {
        inputStream.str(console.inputBuffer);
        inputStream.clear(); 
        return inputStream;
    }

    void GLConsole::release() {
        // Release font/TTF resources before TTF_Quit
        console.clear();
        if (!SDL_GL_GetCurrentContext()) {
            texture = 0;
            sprite.reset();
            shader.reset();
            return;
        }
        if(texture && glIsTexture(texture)) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        if(sprite) {
            sprite.reset();
        }
        
        if(shader) {
            shader->release();
            shader.reset();
        }
    }

    std::ostringstream &Console::bufferData() {
        return data;
    }

    void Console::thread_safe_print(const std::string &str) {
        THREAD_GUARD(console_mutex);
        message_queue.push(str);
        needsRedraw = true;
        needsReflow = true;
    }

    void Console::worker_thread_func() {
        while (worker_active) {
            CommandQueue::Command cmd;
            if (command_queue.pop(cmd)) {
                std::ostringstream result;
                try {
                    cmd.callback(cmd.text, result);
                    if (!result.str().empty()) {
                        thread_safe_print(result.str());
                    }
                } catch (const std::exception& e) {
                    thread_safe_print("Error: " + std::string(e.what()) + "\n");
                } catch (...) {
                    thread_safe_print("Unknown error occurred\n");
                }
                
                if (window) {
                    SDL_Event ev;
                    ev.type = SDL_USEREVENT;
                    SDL_PushEvent(&ev);
                }
            }
        }
    }

    void Console::process_message_queue() {
        THREAD_GUARD(console_mutex);
        if(message_queue.empty()) return;
        while(!message_queue.empty()) {
            data << message_queue.front();
            message_queue.pop();
        }
        static constexpr size_t MAX_CHARS = 300000;
        auto s = data.str();
        if (s.size() > MAX_CHARS) {
            s = s.substr(s.size() - MAX_CHARS);
            data.str("");
            data << s;
        }
        needsReflow = needsRedraw = true;
        scrollToBottom();
    }
    void Console::setInputCallback(std::function<int(gl::GLWindow *win, const std::string &)> callback) {
        THREAD_GUARD(console_mutex);
        callbackEnter = callback;
        this->enterCallbackSet = true;
    }

    void Console::procCmd(const std::string &cmd_text) {
        
        if (cmd_text.empty()) {
            return;
        }
        
        if (!inMultilineMode || braceCount == 0) {
            if (commandHistory.empty() || commandHistory.back() != cmd_text) {
                commandHistory.push_back(cmd_text);
                const size_t MAX_HISTORY = 50;
                if (commandHistory.size() > MAX_HISTORY) {
                    commandHistory.erase(commandHistory.begin());
                }
            }
        }
        
        historyIndex = -1;
        tempBuffer.clear();
        std::vector<std::string> tokens = tokenize(cmd_text);
        if (callbackEnter != nullptr && window != nullptr && enterCallbackSet) {
            command_queue.push({cmd_text, [this, cmd_text](const std::string& text, std::ostream& output) {
                if (callbackEnter && window) {
                    callbackEnter(window, text);
                }
            }});
#if !THREAD_ENABLED
            processCommandQueue();
#endif
            
            needsReflow = true;
        } 
        else if (callbackSet && window != nullptr) {
            if (!callback(this->window, tokens)) {
                needsReflow = true;
            }
        }

        updateCursorPosition();
        checkScroll();
    }

#ifdef __EMSCRIPTEN__
    void Console::processCommandQueue() {
#if !THREAD_ENABLED
        CommandQueue::Command cmd;
        while (command_queue.pop(cmd)) {
            std::ostringstream result;
            try {
                cmd.callback(cmd.text, result);
                if (!result.str().empty()) {
                    thread_safe_print(result.str());
                }
            } catch (const std::exception& e) {
                thread_safe_print("Error: " + std::string(e.what()) + "\n");
            } catch (...) {
                thread_safe_print("Unknown error occurred\n");
            }
            
            if (window) {
                SDL_Event ev;
                ev.type = SDL_USEREVENT;
                SDL_PushEvent(&ev);
            }
        }
#endif
    }
#endif
    void Console::startFadeIn() {
        fadeState = FADE_IN;
        fadeStartTime = SDL_GetTicks();
        if (alpha == 0) 
            alpha = 1;
    }

    void Console::clearText() {
        THREAD_GUARD(console_mutex);
        data.str("");
        data.clear();
        lines.clear();
        needsReflow = true;
        needsRedraw = true;
    }

    void Console::startFadeOut() {
        fadeState = FADE_OUT;
        fadeStartTime = SDL_GetTicks();
    }

    void Console::show() {
        startFadeIn();
    }
        
    void Console::hide() {
        startFadeOut();
    }

    void Console::setTextAttrib(const int size, const SDL_Color &col) {
        THREAD_GUARD(console_mutex);
        font_size = size;
        color = col;
        reload();
    }

    void GLConsole::show() {
        console.startFadeIn();
    }

    void GLConsole::hide() {
        console.startFadeOut();
    }

    GLConsole::GLConsole() {
        rc = {0, 0, 0, 0};
    }

    GLConsole::~GLConsole() {
        release();
    }

    void GLConsole::clearText() {
        console.clearText();
    }

    void GLConsole::thread_safe_print(const std::string &data) {
        console.thread_safe_print(data);
    }

    void GLConsole::process_message_queue() {
        console.process_message_queue();
    }

    void GLConsole::setInputCallback(std::function<int(gl::GLWindow *window, const std::string &)> callback) {
        console.setInputCallback(callback);
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

    void GLConsole::load(gl::GLWindow *win, const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &col) {
        font = fnt;
        font_size = size;
        color = col;
        console.util = &win->util;
        this->rc = rc;
        stretch_height = 0;
        console.create(rc.x, rc.y, rc.w, rc.h);
        if(console.loaded()) {
            console.reload();
        } else {
            console.load(fnt, size, col);
        }
        console.promptText = "$ ";
        shader = std::make_unique<gl::ShaderProgram>();
        if(!shader->loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Error loading shader");
        }
        if(sprite == nullptr)
            sprite = std::make_unique<gl::GLSprite>();
       else
            sprite.reset(new gl::GLSprite());

        texture = loadTextFromSurface(console.drawText());
        sprite->initSize(win->w, win->h);
        sprite->initWithTexture(shader.get(), texture, 0.0f, 0.0f, win->w, win->h);
        stretch_value = true;
    }
    
    void GLConsole::setTextAttrib(const int size, const SDL_Color &col) {;
        console.setTextAttrib(size, col);
    }
    void GLConsole::load(gl::GLWindow *win, const std::string &fnt, int size, const SDL_Color &col) {
        font = fnt;
        font_size = size;
        color = col;
        int consoleWidth = win->w - 50;
        int consoleHeight = (win->h / 2) - 50;
        SDL_Rect rc;
        rc.x = 25;
        rc.y = 25;
        rc.w = consoleWidth;
        rc.h = consoleHeight;
        this->rc = rc;
        console.util = &win->util;
        load(win, rc, fnt, size, col);
        stretch_height = 1;
        stretch_value = true;
    }

    void GLConsole::updateTexture(GLuint tex, SDL_Surface *surf) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLConsole::draw(gl::GLWindow *win) {
        Uint32 currentTime = SDL_GetTicks();
        
        
        if (console.fadeState != FADE_NONE) {
            console.needsRedraw = true;
        }
        
        
        if (currentTime - console.cursorBlinkTime > 500) {
            console.cursorVisible = !console.cursorVisible;
            console.cursorBlinkTime = currentTime;
            console.needsRedraw = true; 
        }

        SDL_Surface *surface = nullptr;
        if (console.needsRedraw) {
            surface = console.drawText();
            if (surface) {
                shader->useProgram();
                shader->setUniform("textTexture", 0);
                updateTexture(texture, surface);
            }
        }

        int consoleWidth = console.getWidth();
        int consoleHeight = console.getHeight();
        sprite->draw(texture, 25.0f, 25.0f, consoleWidth, consoleHeight);
    }

    void GLConsole::print(const std::string &data) {
        console.thread_safe_print(data);
        SDL_Event ev{SDL_USEREVENT};
        SDL_PushEvent(&ev);
    }

    void GLConsole::println(const std::string &data) {
        print(data + "\n");
    }

    void GLConsole::event(gl::GLWindow *win, SDL_Event &e) {
        if(e.type == SDL_TEXTINPUT) {
            for(int i = 0; i < SDL_TEXTINPUTEVENT_TEXT_SIZE && e.text.text[i] != '\0'; ++i) {
                console.keypress(e.text.text[i]);
            }
            return;
        }
        if(e.type == SDL_KEYDOWN) {
            console.needsRedraw = true;
            if(e.key.keysym.sym == SDLK_ESCAPE) {
                win->quit();
            } else if(e.key.keysym.sym == SDLK_RETURN) {
                console.keypress(13);
            } else if(e.key.keysym.sym == SDLK_BACKSPACE) {
                console.keypress(8);
            } else if(e.key.keysym.sym == SDLK_TAB) {
                console.keypress('\t');
            } else if(e.key.keysym.sym == SDLK_LEFT) {
                console.moveCursorLeft();
                return;
            } else if(e.key.keysym.sym == SDLK_RIGHT) {
                console.moveCursorRight();
                return;
            } else if(e.key.keysym.sym == SDLK_UP) {
                console.moveHistoryUp();
                return;
            } else if(e.key.keysym.sym == SDLK_DOWN) {
                console.moveHistoryDown();
                return;
            } 
            else if(e.key.keysym.sym == SDLK_PAGEUP) {
                console.scrollPageUp();
                return;
            }
            else if(e.key.keysym.sym == SDLK_PAGEDOWN) {
                console.scrollPageDown();
                return;
            } else if(e.key.keysym.sym == SDLK_DELETE) {  
                console.keypress(127);  
            }
        }
        else if(e.type == SDL_MOUSEWHEEL) {
            if (win->console_visible) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                console.handleMouseScroll(x, y, e.wheel.y);
                return;
            }
        }
        else if(e.type == SDL_MOUSEBUTTONDOWN) {
            if (win->console_visible && e.button.button == SDL_BUTTON_LEFT) {
                if (console.isMouseOverScrollbar(e.button.x, e.button.y)) {
                    console.beginScrollDrag(e.button.y);
                    return;
                }
            }
        }
        else if(e.type == SDL_MOUSEMOTION) {
            if (win->console_visible && console.scrollDragging) { 
                console.updateScrollDrag(e.motion.y);
                return;
            }
        }
        else if(e.type == SDL_MOUSEBUTTONUP) {
            if (win->console_visible && e.button.button == SDL_BUTTON_LEFT) {
                if (console.scrollDragging) {
                    console.endScrollDrag();
                }
                return;
            }
        }
        
#if defined(__EMSCRIPTEN__)
        refresh();
#endif
    }

    void GLConsole::setStretchHeight(int value) {
        stretch_height = value;
    }

    void GLConsole::resize(gl::GLWindow *win, int w, int h) {
        if(stretch_value)  {
            rc.x = 25;
            rc.y = 25;
            rc.w = w - 50;
            if(stretch_height == 1)
                rc.h = (h / 2) - 50;
            else
                rc.h = h - 50;
        }
        
        if(!font.empty()) 
            load(win, rc, font, font_size, color);

        console.needsRedraw = true;
    }

    void GLConsole::setCallback(gl::GLWindow *window, std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback) {
        console.setCallback(window, callback);
    }

    void GLConsole::setPrompt(const std::string &p) {
        console.promptText = p;
    }
    int GLConsole::getWidth() const { return console.getWidth(); }
    int GLConsole::getHeight() const { return console.getHeight(); }  

    bool GLConsole::procDefaultCommands(const std::vector<std::string> &cmd) {
        if (cmd.empty()) {
            return true;
        } else if(cmd.size() == 1 && (cmd[0] == "about" || cmd[0] == "version")) {
           print("MX2 version " + std::to_string(PROJECT_VERSION_MAJOR) + "." + std::to_string(PROJECT_VERSION_MINOR) + "\n");
           print("writen by Jared Bruni\n");
           print("https://lostsidedead.biz\n");
        } 
        else if (cmd.size() > 0 && cmd[0] == "clear") {
            console.clearText();
            return true;
        } else if (cmd.size() > 0 && cmd[0] == "exit") {
            print("Exiting...\n");
            if (console.window) {
                console.window->quit();
            }
            return true;
        } else if(cmd.size() == 5 && cmd[0] == "settext") {
                int size = std::stoi(cmd[1]);
                SDL_Color color;
                color.r = std::stoi(cmd[2]);
                color.g = std::stoi(cmd[3]);
                color.b = std::stoi(cmd[4]);
                color.a = 255;
                setTextAttrib(size, color);
                print("Text attributes set to size " + std::to_string(size) + 
                        " and color (" + std::to_string(color.r) + ", " + 
                        std::to_string(color.g) + ", " + std::to_string(color.b) + ")\n");
                return true;
        } else {
                print("Unknown command: " + cmd[0] + "\n");
                return false;
        }
        return false;
    }

    void Console::scrollPageUp() {
        THREAD_GUARD(console_mutex);
        int pageSize = std::max(1, visibleLineCount - 1);
        int maxOffset = std::max(0, (int)lines.size() - visibleLineCount);
        scrollOffset = std::min(maxOffset, scrollOffset + pageSize);
        needsRedraw = true;
    }

    void Console::scrollPageDown() {
        THREAD_GUARD(console_mutex);
        int pageSize = std::max(1, visibleLineCount - 1);
        scrollOffset = std::max(0, scrollOffset - pageSize);
        needsRedraw = true;
    }

    bool Console::isMouseOverScrollbar(int mouseX, int mouseY) const {
        bool hit = mouseX >= scrollbarRect.x
                && mouseX <  scrollbarRect.x + scrollbarRect.w
                && mouseY >= scrollbarRect.y
                && mouseY <  scrollbarRect.y + scrollbarRect.h;
        return hit;
    }

    void Console::beginScrollDrag(int mouseY) {
        THREAD_GUARD(console_mutex);
        scrollDragging = true;
        scrollDragStartY = mouseY; 
        scrollDragStartOffset = scrollOffset;
    }

    void Console::updateScrollDrag(int mouseY) {
        THREAD_GUARD(console_mutex);
        if (!scrollDragging) return;
        int total = lines.size();
        if (total <= visibleLineCount) return;

            int totalLines = lines.size();
            if (totalLines <= visibleLineCount) return;

            
            int thumbMinH = 20;
            int trackH    = scrollbarRect.h;

            
            int localY = mouseY - scrollbarRect.y;
            int maxLocal = trackH - thumbMinH;
            localY = std::max(0, std::min(maxLocal, localY));

            float progress = 1.0f - (float)localY / (float)maxLocal;
            int maxOffset = std::max(0, total - visibleLineCount);
            scrollOffset = (int)(progress * maxOffset);
            scrollOffset = std::max(0, std::min(maxOffset, scrollOffset));
            needsRedraw = true;
    }

    void Console::endScrollDrag() {
        THREAD_GUARD(console_mutex);
        scrollDragging = false;
    }

    void Console::handleMouseScroll(int mouseX, int mouseY, int wheelY) {
        THREAD_GUARD(console_mutex);
        if (wheelY > 0) {
            scrollUp();
        } else if (wheelY < 0) {
            scrollDown();
        }
    }
}
