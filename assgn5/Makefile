# Author: Christina Sardo (csardo) and Lance Boettcher (lboettch)
# Section: cpe453-01
# This is the Makefile for assignment 5

all: minls minget

minls: minls.c filesystem.h inode.c superblock.c
	gcc -Wall -Werror -o minls inode.c superblock.c minls.c

minget: minget.c filesystem.h
	gcc -Wall -Werror -o minget minget.c

clean:
	rm minls minget
