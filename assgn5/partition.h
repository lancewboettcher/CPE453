#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "filesystem.h"
#endif

extern int getPartitionTable(FILE *, struct partitionEntry **,
	  int, uint32_t);
extern void printPartitionTable(struct partitionEntry*,
      struct partitionEntry*);
extern int checkPartitionMagic(FILE*, struct partitionEntry*, int);
