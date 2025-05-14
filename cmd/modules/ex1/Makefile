all:
	$(CXX) -std=c++20 -I/usr/local/include/mxcmd mod.cpp -shared -o libmodule.so -L/usr/local/lib -lmxcmd -ldl -fPIC
install:
	cp -rf libmodule.so /usr/local/lib
clean:
	rm libmodule.so
