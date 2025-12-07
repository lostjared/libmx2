CXX = em++
CXXFLAGS = -std=c++20 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png","jpg"]'
LIBS_PATH = /home/jared/emscripten-libs
ZLIB_INCLUDE = -s USE_ZLIB=1
PNG_INCLUDE = -s USE_LIBPNG=1
MX_INCLUDE = -I$(LIBS_PATH)/mx2/include -I/usr/include/glm
ZLIB_LIB = -s USE_ZLIB=1
PNG_LIB = -s USE_LIBPNG=1
LIBMX_LIB = $(LIBS_PATH)/mx2/lib/libmx.a 
PRELOAD = --preload-file data
SOURCES = about.cpp
OBJECTS = $(SOURCES:.cpp=.o)
OUTPUT = MX_app.html

.PHONY: all clean install

all: $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(MX_INCLUDE) $(ZLIB_INCLUDE) $(PNG_INCLUDE) -c $< -o $@

$(OUTPUT): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(OUTPUT) $(PRELOAD)  -s USE_SDL=2 -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png","jpg"]' -s USE_SDL_TTF=2 $(LIBMX_LIB) $(PNG_LIB) $(ZLIB_LIB) -s ALLOW_MEMORY_GROWTH -s ASSERTIONS -s ENVIRONMENT=web -s USE_WEBGL2=1 -s FULL_ES3 -s USE_SDL_MIXER=2 -lembind

clean:
	rm -f *.o $(OUTPUT) *.wasm *.js *.data
