all:
	g++ -std=c++11 gencode.console.cpp -o shader-gen
install: all
	cp shader-gen /usr/local/bin
clean:
	rm -f shader-gen
