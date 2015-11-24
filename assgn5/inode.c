#include "filesystem.h"

struct inode* getInode() {
   struct inode *node = malloc(sizeof(struct inode));

   return node;
}

void printInode() {
   printf("hey I got in\n");
}
