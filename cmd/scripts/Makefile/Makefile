# Generated Makefile

CC= gcc
CFLAGS= -std=c11 -Wall -pedantic
LDFLAGS= 
OBJECTS =  test.o main.o
install: HelloWorld
HelloWorld: $(OBJECTS)
	$(CC) $(OBJECTS) -o HelloWorld $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJECTS) HelloWorld
