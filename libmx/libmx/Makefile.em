CXX = em++
CXXFLAGS = -std=c++20 -O2 -DFOR_WASM -s USE_SDL=2 -s USE_SDL_TTF=2
LIBS_PATH = /home/jared/emscripten-libs
ZLIB_INCLUDE = -I$(LIBS_PATH)/zlib/include
PNG_INCLUDE = -I$(LIBS_PATH)/libpng/include
ZLIB_LIB = $(LIBS_PATH)/zlib/lib/libz.a
PNG_LIB = $(LIBS_PATH)/libpng/lib/libpng.a
SOURCES = cfg.cpp exception.cpp font.cpp loadpng.cpp mx.cpp texture.cpp util.cpp
OBJECTS = $(SOURCES:.cpp=.o)
OUTPUT = libmx.a

.PHONY: all clean install

all: $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(ZLIB_INCLUDE) $(PNG_INCLUDE) -c $< -o $@

$(OUTPUT): $(OBJECTS)
	emar cru $(OUTPUT) $(OBJECTS)

install: all
	mkdir -p $(LIBS_PATH)/mx2/include
	mkdir -p $(LIBS_PATH)/mx2/lib
	cp -rf *.hpp $(LIBS_PATH)/mx2/include
	cp -rf $(OUTPUT) $(LIBS_PATH)/mx2/lib

clean:
	rm -f *.o $(OUTPUT)