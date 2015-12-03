#include "filesystem.h"

#ifndef INODE_H
#include "inode.h"
#endif

#ifndef SUPERBLOCK_H
#include "superblock.h"
#endif

#ifndef PARTITION_H
#include "partition.h"
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

void seekPastPartitions(FILE *fileImage,
                        struct partitionEntry *partitionTable,
                        int whichPartition,
                        struct partitionEntry *subPartitionTable,
                        int whichSubPartition) {
   /* Set the file pointer to the beginning of the file */
   fseek(fileImage, 0, SEEK_SET);

   /* If there is a partition table, seek past it */
   if (whichPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition].lFirst,
            SEEK_SET);
   }

   /* If there is a subpartition table, seek past it */
   if (whichSubPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * 
            subPartitionTable[whichSubPartition].lFirst, SEEK_SET);
   }

}

void printVerbose(struct partitionEntry *partitionTable,
                  struct partitionEntry *subPartitionTable,
                  struct superblock *superBlock,
                  struct inode *node) {
   /* Print partition/subpartition tables if any */
   printPartitionTable(partitionTable, subPartitionTable);

   /* Print superblokc contents */
   printSuperblock(superBlock);

   /* Print inode */
   printInode(node);
}



struct inode* getDirectory(FILE *fileImage,
                           struct inode *dirNode,
                           struct superblock *superBlock,
                           struct partitionEntry *partitionTable,
                           int whichPartition,
                           struct partitionEntry *subPartitionTable,
                           int whichSubPartition,
                           char *nextDir) {
      int numDirectories, numZones, zoneIdx = 0;
      char *directoryName;
      struct inode *nextDirNode = NULL;
      struct directoryEntry *directory = malloc(sizeof(struct inode));
                  
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);

      /* Check to see how many zones we'll need to read */
      /* if size of data is greater than the amount of space per zone,
       * the we're going to need to traverse multiple blocks.
       * NOTE: Need to add one to round up sizes in case of decimals */
      numZones = getNumberOfZones(fileImage, superBlock, dirNode);
      
      if (numZones > DIRECT_ZONES) {
         numZones = DIRECT_ZONES;
      }

      while(numZones--) {
                                
         /* Calculate the number of directories in this data zone */
         if (numZones) {
            numDirectories = entriesPerZone(superBlock);
         }
         else {
            numDirectories = dirNode->size / DIRECTORY_ENTRY_SIZE;

            if ((numDirectories * DIRECTORY_ENTRY_SIZE) >
                  zoneSize(superBlock)) {
               if (dirNode->indirect) {
                  numDirectories = (zoneSize(superBlock) /
                        DIRECTORY_ENTRY_SIZE);
               }
               else {
                  numDirectories %= DIRECTORY_ENTRY_SIZE;
               }
            }
         }
         
         seekPastPartitions(fileImage, partitionTable, whichPartition,
               subPartitionTable, whichSubPartition);

         /* Navigate to data zone */
         fseek(fileImage, dirNode->zone[zoneIdx] * 
         zoneSize(superBlock), SEEK_CUR);
         /* Read through the directory names and return the inode of the 
          * matched directory */
         while (numDirectories--) {
            fread(directory, sizeof(struct directoryEntry), 1, fileImage);
         
            directoryName = (char *)directory->filename;
            //printf("%s\n", dirName);
            if (!strcmp(directoryName, nextDir) && directory->inode) {
               nextDirNode = getInode(fileImage, superBlock, whichPartition,
                  partitionTable, whichSubPartition, subPartitionTable,
                  directory->inode);      
            }  
         }

         /* Increment zone index to navigate to next zone */
         zoneIdx++;
      }

      if (dirNode->indirect && !nextDirNode) {
         struct directoryEntry *indirectDirectory = getIndirectBlock(fileImage,
               dirNode, superBlock, partitionTable, whichPartition,
               subPartitionTable, whichSubPartition, nextDir);

         if (indirectDirectory != NULL) {
            nextDirNode = getInode(fileImage, superBlock, whichPartition,
                  partitionTable, whichSubPartition, subPartitionTable,
                  indirectDirectory->inode);  
         }
      }
      return nextDirNode;
}

struct directoryEntry *getIndirectBlock(FILE *fileImage,
                                    struct inode *node,
                                    struct superblock *superBlock,
                                    struct partitionEntry *partitionTable,
                                    int whichPartition,
                                    struct partitionEntry *subPartitionTable,
                                    int whichSubPartition,
                                    char *fileToGet) {

   struct directoryEntry *dir = malloc(sizeof(struct directoryEntry)),
      *fileFound = NULL;
   struct inode *tempNode;
   char *dirName;
   
   /* Set file pointer to past the partitions (if any) */
   seekPastPartitions(fileImage, partitionTable, whichPartition,
      subPartitionTable, whichSubPartition); 

   fseek(fileImage, node->indirect * zoneSize(superBlock), SEEK_CUR);
   
   int numIndirectZones = (node->size / zoneSize(superBlock) ) + 1;
   numIndirectZones -= DIRECT_ZONES;

   uint32_t *indirectZones[numIndirectZones];
   int i, numEntries;

   for (i = 0; i < numIndirectZones; i++) {
      indirectZones[i] = malloc(sizeof(uint32_t));
      fread(indirectZones[i], sizeof(uint32_t), 1, fileImage);
   }
   
   for (i = 0; i < numIndirectZones; i++) {     
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition); 

      fseek(fileImage, *indirectZones[i] * zoneSize(superBlock), 
         SEEK_CUR);

      numEntries = entriesPerZone(superBlock);
      
      while (numEntries--) {      
         fread(dir, sizeof(struct directoryEntry), 1, fileImage);
         
         dirName = (char*)dir->filename;

         if (dir->inode != 0) {
            tempNode = getInode(fileImage, superBlock, whichPartition,
               partitionTable, whichSubPartition, subPartitionTable,
               dir->inode);
            
            if (fileToGet == NULL) {
               printf("%s\t%u %s\n", getPermissions(tempNode->mode), 
                  tempNode->size, dir->filename);
            }
            else {
               if (!strcmp(dirName, fileToGet)) {
                  return dir;
               }
            }

            free(tempNode);
         }
      }
   }

   return fileFound;
}

int navigatePath(FILE *fileImage,
                 struct inode **origNode,
                 struct superblock *superBlock,
                 struct partitionEntry *partitionTable,
                 int whichPartition,
                 struct partitionEntry *subPartitionTable,
                 int whichSubPartition,
                 char *path) {
   char *originalPath = strdup(path);
   char *nextDir = "";
   struct inode *nextNode;

   while (!strcmp(nextDir, "")) {
         nextDir = strsep(&originalPath, "/");
      }
   
   while (nextDir) {
      if (!((*origNode)->mode & DIRECTORY_MASK)) {
         fprintf(stderr, "lookupdir: Not a directory\n");

         return NO_FILE_FOUND;
      }
      //printf("41ff %x\n", (*origNode)->mode & FILE_TYPE_MASK
      //& REGULAR_FILE_MASK);
      nextNode = getDirectory(fileImage, *origNode, superBlock,
                           partitionTable, whichPartition, subPartitionTable,
                           whichSubPartition, nextDir);
        
         if (nextNode == NULL) {
                  return NO_FILE_FOUND;
                  
         }
         
      //printf("a1ff %x\n", nextNode->mode & FILE_TYPE_MASK 
      //& REGULAR_FILE_MASK);
         free(*origNode);
         *origNode = nextNode;
         nextDir = strsep(&originalPath, "/");
      }
   
   return EXIT_SUCCESS;
}

int getNumberOfZones(FILE* fileImage,
                     struct superblock *superBlock,
                     struct inode *origNode) {
   int numZones;
         
   if (origNode->size % zoneSize(superBlock)) {
      numZones = (origNode->size / zoneSize(superBlock)) + 1;
   }
   else {
      numZones = (origNode->size / zoneSize(superBlock));
   }

   return numZones;
}
