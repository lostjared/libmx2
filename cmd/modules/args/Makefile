OUTPUT=libargs.so
SRC=mod.cpp

all:
	$(CXX) -std=c++20 -I/usr/local/include/mxcmd $(SRC) -shared -o $(OUTPUT) -L/usr/local/lib -lmxcmd -ldl -fPIC
install:
	cp -rf $(OUTPUT) /usr/local/lib
clean:
	rm $(OUTPUT)
