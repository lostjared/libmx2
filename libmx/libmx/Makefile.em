CXX = em++
CXXFLAGS = -std=c++20 -O2 -DFOR_WASM -DWITH_MIXER -DWITH_GL -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 -I/usr/local/include/glm -I/usr/include/glm
LIBS_PATH = $(HOME)/emscripten-libs
ZLIB_INCLUDE = -s USE_ZLIB=1 #-I$(LIBS_PATH)/zlib/include
PNG_INCLUDE = -s USE_LIBPNG=1 #-I$(LIBS_PATH)/libpng/include
ZLIB_LIB = -s USE_ZLIB=1 #$(LIBS_PATH)/zlib/lib/libz.a
PNG_LIB = -s USE_LIBPNG=1 #$(LIBS_PATH)/libpng/lib/libpng.a
SOURCES = cfg.cpp exception.cpp font.cpp loadpng.cpp mx.cpp texture.cpp util.cpp joystick.cpp gl.cpp input.cpp sound.cpp model.cpp tee_stream.cpp console.cpp
OBJECTS = $(SOURCES:.cpp=.o)
OUTPUT = libmx.a

.PHONY: all clean install

all: $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(ZLIB_INCLUDE) $(PNG_INCLUDE) -I/usr/include/glm -c $< -o $@

$(OUTPUT): $(OBJECTS)
	emar cru $(OUTPUT) $(OBJECTS)

install: all
	mkdir -p $(LIBS_PATH)/mx2/include
	mkdir -p $(LIBS_PATH)/mx2/lib
	cp -rf *.hpp $(LIBS_PATH)/mx2/include
	cp -rf $(OUTPUT) $(LIBS_PATH)/mx2/lib

clean:
	rm -f *.o $(OUTPUT)
