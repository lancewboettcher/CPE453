#include "partition.h"

#define PARTITION_H


int getPartitionTable(FILE *fileImage, 
	                  struct partitionEntry **partitionTable,
	                  int whichPartition, 
	                  uint32_t offset) {
   /* If a partitition was specified, check the partition table for validity */
   if (whichPartition >= 0) {
      *partitionTable = malloc(sizeof(struct partitionEntry) * 
      	NUM_PRIMARY_PARTITIONS);

      fseek(fileImage, SECTOR_SIZE * offset, SEEK_SET);

      /* Seek to partition sector and read the table */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_CUR);

      fread(*partitionTable, sizeof(struct partitionEntry) * 
      	NUM_PRIMARY_PARTITIONS, 1, fileImage);
      
      if (!offset && (*partitionTable + whichPartition)->type != 
      	MINIX_PARTITION_TYPE) {
         fprintf(stderr, "Not a Minix partition\n");

         return INVALID_PARTITION;
      }

   }

   return EXIT_SUCCESS;
}

void printPartitionTable(struct partitionEntry *partitionTable,
                         struct partitionEntry *subPartitionTable) {  
   int i;

   if (partitionTable != NULL) {
      printf("\nPartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      "
               "First       Size\n");
      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         printf("  0x%.02x    %u    %u    %u 0x%.02x    %u   "
                  "%u  %u         %u      %u\n",
                  partitionTable[i].bootind, 
                  partitionTable[i].start_head, 
                  partitionTable[i].start_sec,
                  partitionTable[i].start_cyl, 
                  partitionTable[i].type, 
                  partitionTable[i].end_head,
                  partitionTable[i].end_sec & 0x3F, 
                  ((partitionTable[i].end_sec & 0xC0)<<2) + 
                  partitionTable[i].end_cyl, 
                  partitionTable[i].lFirst,
                  partitionTable[i].size);
      }
   }
   
   if (subPartitionTable != NULL) {
      printf("\nSubpartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      "
               "First       Size\n");
      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         printf("  0x%.02x    %u    %u    %u 0x%.02x    %u   %u  "
                  "%u         %u      %u\n",
                  subPartitionTable[i].bootind, 
                  subPartitionTable[i].start_head, 
                  subPartitionTable[i].start_sec,
                  subPartitionTable[i].start_cyl, 
                  subPartitionTable[i].type, 
                  subPartitionTable[i].end_head,
                  subPartitionTable[i].end_sec & 0x3F, 
                  ((subPartitionTable[i].end_sec & 0xC0)<<2) + 
                  subPartitionTable[i].end_cyl, 
                  partitionTable[i].lFirst, 
                  subPartitionTable[i].size);
      }
   }
}

int checkPartitionMagic(FILE *fileImage,
                        struct partitionEntry *partitionTable,
                        int whichPartition) {
   uint8_t bootSectValidation510, bootSectValidation511;

   /* Seek to the sector that lFirst of the specified partition points to */
   fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition].lFirst, 
   SEEK_SET);
   
   /* Read in byte 510 amd byte 511 */
   fseek(fileImage, BOOT_SECTOR_BYTE_510, SEEK_CUR);
   fread(&bootSectValidation510, sizeof(uint8_t), 1, fileImage);
   fread(&bootSectValidation511, sizeof(uint8_t), 1, fileImage);
   
   /* Check that t bytes match the minix magic bytes */
   if (bootSectValidation510 != BOOT_SECTOR_BYTE_510_VAL || 
         bootSectValidation511 != BOOT_SECTOR_BYTE_511_VAL) {
      fprintf(stderr, "Partition table does not contain a "
            "valid signature\n");
      return INVALID_PARTITION;
   }

   return EXIT_SUCCESS;
}
