malloc: malloc.c
	gcc -Wall -g -c -o tryme.o tryme.c
	gcc -L ~pn-cs453/demos/lib -lmalloc -o tryme tryme.o

intel-all: malloc.c
	mkdir lib
	gcc -Wall -g -fpic -m32 -c -o malloc32.o malloc.c
	gcc -Wall -g -fpic -m32 -shared -o lib/libmalloc.so malloc32.o
	mkdir lib64
	gcc -Wall -g -fpic  -c -o malloc64.o malloc.c
	gcc -Wall -g -fpic shared -o lib64/libmalloc.so malloc64.o

clean:
	rm -f malloc.o *~ TAGS