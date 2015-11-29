#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "filesystem.h"
#endif

#define INODE_H

extern struct inode* getInode(FILE*, struct superblock*, int, 
      struct partitionEntry**, int, struct partitionEntry**, uint32_t);
extern struct inode* getIndirectInode(FILE*, struct superblock*, int, 
      struct partitionEntry**, int, struct partitionEntry**, uint32_t);
extern void printInode(struct inode*);
extern char* getPermissions(uint16_t);
