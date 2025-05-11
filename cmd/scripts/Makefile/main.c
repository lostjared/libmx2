#include<stdio.h>

extern const char  *hello;

int main(int argc, char **argv) {
	printf("%s\n", hello);
	return 0;
}
