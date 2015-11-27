#include "filesystem.h"

#ifndef INODE_H
#include "inode.h"
#endif

#ifndef SUPERBLOCK_H
#include "superblock.h"
#endif

void printHelp() {
   printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
   printf("Options:\n");
   printf("-p part    --- select partition for filesystem "
         "(default: none)\n");
   printf("-s sub     --- select subpartition for filesystem "
       "(default: none)\n");
   printf("-h help    --- print usage information and exit\n");
   printf("-v verbose --- increase verbosity level\n");
}

void printPartitionTable(struct partitionEntry **partitionTable,
                         struct partitionEntry **subPartitionTable) {  
   int i;

   if (*partitionTable != NULL) {
      printf("\nSubpartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      "
               "First       Size\n");
      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         printf("  0x%.02x    %u    %u    %u 0x%.02x    %u   "
                  "%u  %u         %u      %u\n",
                  partitionTable[i]->bootind, 
                  partitionTable[i]->start_head, 
                  partitionTable[i]->start_sec,
                  partitionTable[i]->start_cyl, 
                  partitionTable[i]->type, 
                  partitionTable[i]->end_head,
                  partitionTable[i]->end_sec & 0x3F, 
                  ((partitionTable[i]->end_sec & 0xC0)<<2) + 
                  partitionTable[i]->end_cyl, 
                  partitionTable[i]->lFirst,
                  partitionTable[i]->size);
      }
   }
   
   if (*subPartitionTable != NULL) {
      printf("\nPartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      "
               "First       Size\n");
      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         printf("  0x%.02x    %u    %u    %u 0x%.02x    %u   %u  "
                  "%u         %u      %u\n",
                  subPartitionTable[i]->bootind, 
                  subPartitionTable[i]->start_head, 
                  subPartitionTable[i]->start_sec,
                  subPartitionTable[i]->start_cyl, 
                  subPartitionTable[i]->type, 
                  subPartitionTable[i]->end_head,
                  subPartitionTable[i]->end_sec & 0x3F, 
                  ((subPartitionTable[i]->end_sec & 0xC0)<<2) + 
                  subPartitionTable[i]->end_cyl, 
                  partitionTable[i]->lFirst, 
                  subPartitionTable[i]->size);
      }
   }
}

void printVerbose(struct partitionEntry **partitionTable,
                  struct partitionEntry **subPartitionTable,
                  struct superblock *superBlock,
                  struct inode *node) {
   /* Print partition/subpartition tables if any */
   printPartitionTable(partitionTable, subPartitionTable);

   /* Print superblokc contents */
   printSuperblock(superBlock);

   /* Print inode */
   printInode(node);
}

void seekPastPartitions(FILE *fileImage,
                        struct partitionEntry **partitionTable,
                        int whichPartition,
                        struct partitionEntry **subPartitionTable,
                        int whichSubPartition) {
   /* Set the file pointer to the beginning of the file */
   fseek(fileImage, 0, SEEK_SET);

   /* If there is a partition table, seek past it */
   if (whichPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition]->lFirst,
            SEEK_SET);
   }

   /* If there is a subpartition table, seek past it */
   if (whichSubPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * 
            subPartitionTable[whichSubPartition]->lFirst, SEEK_SET);
   }

}
