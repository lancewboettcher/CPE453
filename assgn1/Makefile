ARCH := $(shell getconf LONG_BIT)

LIB_32 := ~/CPE453/assgn1/lib
LIB_64 := ~/CPE453/assgn1/lib64

LIB := $(LIB_$(ARCH))

malloc: malloc.c tryme.c
	gcc -Wall -g -c -o tryme.o tryme.c
	gcc -L $(LIB) -lmalloc -o tryme tryme.o

intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib malloc32.o
	gcc -Wall -g -fpic -m32 -shared -o lib/libmalloc.so malloc32.o

lib64/libmalloc.so: lib64 malloc64.o 
	gcc -Wall -g -fpic -shared -o lib64/libmalloc.so malloc64.o

lib: 
	mkdir lib

lib64: 
	mkdir lib64

malloc32.o: malloc.c
	gcc -Wall -g -fpic -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	gcc -Wall -g -fpic -c -o malloc64.o malloc.c

clean:
	rm -f malloc32.o malloc64.o
	rm -f tryme.o 
	rm -r lib lib64
