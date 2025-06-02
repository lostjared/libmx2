all:
	g++ -std=c++11 gencode.console.cpp -o shader-gen -lcurl
install: all
	cp shader-gen /usr/local/bin
clean:
	rm -f shader-gen
