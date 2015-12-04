# CPE 453
# Assignment 6 - Hello 
# Lance Boettcher (lboettch) 
#
# Makefile

CC = gcc

CFLAGS = -Wall -ansi -pedantic -g

LD = gcc

LDFLAGS = -g

hello: hello.o
	$(LD) $(LDFLAGS) -o hello hello.o

hello.o: hello.c
	$(CC) $(CFLAGS) -c -o hello.o hello.c

test: hello
	@echo Testing hello...
	@./hello
	@echo done.

clean: 
	rm -f hello.o *~ TAGS
