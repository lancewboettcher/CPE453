#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "filesystem.h"
#endif

extern void printSuperblock(struct superblock*);
extern struct superblock* getSuperblock(FILE*, int, 
      struct partitionEntry*, int, struct partitionEntry*);
extern uint32_t zoneSize(struct superblock*);
extern uint32_t entriesPerZone(struct superblock*);
extern uint32_t pointersPerZone(struct superblock*);
