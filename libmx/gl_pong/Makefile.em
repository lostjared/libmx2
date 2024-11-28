CXX = em++
CXXFLAGS = -std=c++20 -O2 -DFOR_WASM -s USE_SDL=2 -s USE_SDL_TTF=2 
LIBS_PATH = /home/jared/emscripten-libs
ZLIB_INCLUDE = -I$(LIBS_PATH)/zlib/include
PNG_INCLUDE = -I$(LIBS_PATH)/libpng/include
MX_INCLUDE = -I$(LIBS_PATH)/mx2/include -I/usr/include/glm
ZLIB_LIB = $(LIBS_PATH)/zlib/lib/libz.a
PNG_LIB = $(LIBS_PATH)/libpng/lib/libpng.a
LIBMX_LIB = $(LIBS_PATH)/mx2/lib/libmx.a 
PRELOAD = --preload-file data
SOURCES = pong.cpp
OBJECTS = $(SOURCES:.cpp=.o)
OUTPUT = Pong.html

.PHONY: all clean install

all: $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(MX_INCLUDE) $(ZLIB_INCLUDE) $(PNG_INCLUDE) -c $< -o $@

$(OUTPUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(OUTPUT) $(PRELOAD)  -s USE_SDL=2 -s USE_SDL_TTF=2 $(LIBMX_LIB) $(PNG_LIB) $(ZLIB_LIB) -s ALLOW_MEMORY_GROWTH -s ASSERTIONS -s ENVIRONMENT=web 

clean:
	rm -f *.o $(OUTPUT) *.wasm *.js *.data
