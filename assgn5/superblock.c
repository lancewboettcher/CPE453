#include "superblock.h"

#define SUPERBLOCK_H

void printSuperblock(struct superblock *superBlock) {
   printf("\nSuperblock Contents:\n");
   printf("Stored Fields:\n");
   printf("  ninodes        %u\n", superBlock->ninodes);
   printf("  i_blocks       %u\n", superBlock->i_blocks);
   printf("  z_blocks       %u\n", superBlock->z_blocks);
   printf("  firstdata      %u\n", superBlock->firstdata);
   printf("  log_zone_size  %u (zone size: %u)\n",
      superBlock->log_zone_size, 
         superBlock->blocksize << superBlock->log_zone_size);
   printf("  max_file       %u\n", superBlock->max_file);
   printf("  magic          0x%x\n", superBlock->magic);
   printf("  zones          %u\n", superBlock->zones);
   printf("  blocksize      %u\n", superBlock->blocksize);
   printf("  subversion     %u\n", superBlock->subversion);
  
   printf("Computed Fields:\n");
   printf("  version        %u\n", MINIX_VERSION);
   printf("  firstImap      %u\n", INODE_BITMAP_BLOCK);
   printf("  firstZmap      %u\n", ZNODE_BITMAP_BLOCK);
   printf("  firstIblock    %u\n", 2 + 
      superBlock->i_blocks + superBlock->z_blocks);
   printf("  zonesize       %u\n", 
      superBlock->blocksize << superBlock->log_zone_size);
   printf("  ptrs_per_zone  %u\n", PTRS_PER_ZONE);
   printf("  ino_per_block  %u\n", 
      superBlock->blocksize / sizeof(struct inode));
   printf("  wrongended     %u\n", ENDIANNESS);
   printf("  fileent_size   %u\n", DIRECTORY_ENTRY_SIZE);
   printf("  max_filename   %u\n", FILENAME_LENGTH);
   printf("  ent_per_zone   %u\n", (superBlock->blocksize << 
            superBlock->log_zone_size) / DIRECTORY_ENTRY_SIZE);
}

struct superblock* getSuperblock(FILE *fileImage, 
      int whichPartition, struct partitionEntry **partitionTable,
      int whichSubPartition, struct partitionEntry **subPartitionTable) {
   /* Initialize the block */
   struct superblock *superBlock = malloc(sizeof(struct superblock));

   /* Set the file pointer to 0 */
   fseek(fileImage, 0, SEEK_SET);

   /* If no partiton table, seek to the next block (boot block is 2 sectors),
    *     * else use the subpartition specified and seek to that boot block */
   if (whichPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * 
         partitionTable[whichPartition]->lFirst, SEEK_SET);
   }
   
   if (whichSubPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * 
      subPartitionTable[whichSubPartition]->lFirst, SEEK_SET);
   }
      
   /* Seek past the 1KB of boot sector */
   fseek(fileImage, SECTOR_SIZE * 2, SEEK_CUR);

   fread(superBlock, sizeof(struct superblock), 1, fileImage);

   return superBlock;
}

uint32_t zoneSize(struct superblock *superBlock) {
   return superBlock->blocksize << superBlock->log_zone_size;
}

uint32_t entriesPerZone(struct superblock *superBlock) {
   return (superBlock->blocksize << 
            superBlock->log_zone_size) / DIRECTORY_ENTRY_SIZE;
}
