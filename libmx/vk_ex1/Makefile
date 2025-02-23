PATH=/usr/local/opt/molten-vk
INCLUDE=$(PATH)/include
LIB=$(PATH)/lib

all:
	clang++ -std=c++20 -I$(INCLUDE) `/usr/local/bin/sdl2-config --cflags`  -o main.cpp vk.cpp exception.cpp -o vk_ex1 `/usr/local/bin/sdl2-config --libs` -L$(LIB) -framework QuartzCore -framework Metal -framework IOSurface -framework CoreGraphics -lMoltenVK 
	
