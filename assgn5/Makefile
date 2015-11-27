# Author: Christina Sardo (csardo) and Lance Boettcher (lboettch)
# Section: cpe453-01
# This is the Makefile for assignment 5

all: minls minget

minls: minls.c filesystem.h inode.c superblock.c filesystem.c
	gcc -Wall -Werror -o minls filesystem.c inode.c superblock.c minls.c

minget: minget.c filesystem.h inode.c superblock.c filesystem.c
	gcc -Wall -Werror -o minget filesystem.c inode.c superblock.c minget.c

clean:
	rm minls minget
