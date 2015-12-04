#include "inode.h"

struct inode* getInode(FILE *fileImage, 
                       struct superblock *superBlock, 
                       int whichPartition, 
                       struct partitionEntry *partitionTable, 
                       int whichSubPartition, 
                       struct partitionEntry *subPartitionTable, 
                       uint32_t nodeOffset) {
   struct inode *node = malloc(sizeof(struct inode));
  
   /* Need to retain the file pointer for the sake of printing directories */
   uint32_t prevFp = ftell(fileImage);

   /* Set the file pointer past the partitions (if any) */
   seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);

   /* Seek past the superblock and boot blocks */
   fseek(fileImage, superBlock->blocksize * 2, SEEK_CUR);

   /* Seek past the inode bitmap and zone bitmap */
   fseek(fileImage, (superBlock->blocksize * superBlock->z_blocks)
         + (superBlock->blocksize * superBlock->i_blocks), SEEK_CUR);
   
   while (nodeOffset > 1) {
      fseek(fileImage, sizeof(struct inode), SEEK_CUR);
      nodeOffset--;
   }

   fread(node, sizeof(struct inode), 1, fileImage);
   
   /* Return file pointer back to the position it was in before the call */
   fseek(fileImage, prevFp, SEEK_SET);
   return node;
}

struct inode* getIndirectInode(FILE *fileImage, 
                       struct superblock *superBlock, 
                       int whichPartition, 
                       struct partitionEntry *partitionTable, 
                       int whichSubPartition, 
                       struct partitionEntry *subPartitionTable, 
                       uint32_t zone) {
   struct inode *indirectNode = malloc(sizeof(struct inode));

   /* Seek past the partitions */
   seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);

   /* Navigate to the data zone */
   fseek(fileImage, zone * 
         (superBlock->blocksize << superBlock->log_zone_size), SEEK_CUR);
   
   /* Read in the indirect zone */
   fread(indirectNode, sizeof(struct inode), 1, fileImage);

   return indirectNode;
}

void printInode(struct inode *node) {
   /* Print inode */
   printf("\nFile inode:\n");
   printf("  unsigned short mode     0x%x (%s)\n", node->mode,
         getPermissions(node->mode));
   printf("  unsigned short links    %d\n", node->links);
   printf("  unsigned short uid      %d\n", node->uid);
   printf("  unsigned short gid      %d\n", node->gid);
   printf("  uint32_t size           %u\n", node->size);
   printf("  uint32_t atime          %u --- %s\n", node->atime, 
         ctime((time_t*)&node->atime));
   printf("  uint32_t mtime          %u --- %s\n", node->mtime,
         ctime((time_t*)&node->mtime));
   printf("  uint32_t ctime          %u --- %s\n", node->ctime, 
         ctime((time_t*)&node->ctime));
   printf("\n");
   printf("  Direct Zones\n");
   printf("\t\tzone[0]    =     %u\n", node->zone[0]);  
   printf("\t\tzone[1]    =     %u\n", node->zone[1]);
   printf("\t\tzone[2]    =     %u\n", node->zone[2]);  
   printf("\t\tzone[3]    =     %u\n", node->zone[3]);  
   printf("\t\tzone[4]    =     %u\n", node->zone[4]);  
   printf("\t\tzone[5]    =     %u\n", node->zone[5]);  
   printf("\t\tzone[6]    =     %u\n", node->zone[6]);      
   printf("\tuint32_t indirect  =     %u\n", node->indirect);      
   printf("\tuint32_t double    =     %u\n", node->two_indirect);
}

char* getPermissions(uint16_t mode) {
   char *permissions = malloc(sizeof(unsigned char) * PERMISSIONS_LENGTH);
   uint32_t idx = 0;

   memset(permissions, '-', PERMISSIONS_LENGTH);
   /* set directory bit */
   if (mode & DIRECTORY_MASK)
      permissions[idx] = 'd'; 

   idx++;

   /* set owner permissions bits */
   if (mode & OWNER_RD_PERM)
      permissions[idx] = 'r';
   idx++;
   if (mode & OWNER_WR_PERM)
      permissions[idx] = 'w';
   idx++;
   if (mode & OWNER_EX_PERM)
      permissions[idx] = 'x';
   idx++;

   /* set group permissions bits */
   if (mode & GROUP_RD_PERM)
      permissions[idx] = 'r';
   idx++;
   if (mode & GROUP_WR_PERM)
      permissions[idx] = 'w';
   idx++;
   if (mode & GROUP_EX_PERM)
      permissions[idx] = 'x';
   idx++;

   /* set other permissions bits */
   if (mode & OTHER_RD_PERM)
      permissions[idx] = 'r';
   idx++;
   if (mode & OTHER_WR_PERM)
      permissions[idx] = 'w';
   idx++;
   if (mode & OTHER_EX_PERM)
      permissions[idx] = 'x';
   
   return permissions;
}
