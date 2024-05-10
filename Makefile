CC=gcc
CFLAGS=-I. -Wall
DEPS = parser.h
OBJ = main.o parser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

eshell: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o eshell
