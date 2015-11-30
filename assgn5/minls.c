//TODO: read path from command line
//TODO: add 4 primary partition tables
//TODO: add subpartition table functionality
//TODO: check for errors in every malloc
#ifndef FILESYSTEM_H
#include "filesystem.h"
#endif

#ifndef INODE_H
#include "inode.h"
#endif

#ifndef SUPERBLOCK_H
#include "superblock.h"
#endif 

#define TRUE 1
#define FALSE 0
#define BYTESIZE 8
#define BAD_MAGIC_NUMBER -1
#define INVALID_PARTITION -2
#define NO_FILE_FOUND -3

/* Global Variables */
struct superblock *superBlock = NULL;
struct partitionEntry *partitionTable[4] = {NULL};
struct partitionEntry *subPartitionTable[4] = {NULL};
struct inode *node = NULL;

/* Function Prototypes */
void printDirectories(FILE*, int, int, char*);
int initFileSystem(FILE *, int, int, char *);

int main (int argc, char *argv[]) {
   int i = 1;
   int errorMessage;
   int verbose = FALSE;
   int whichPartition = -1, whichSubPartition = -1;
   char *fileName, *pathName = NULL;
   FILE *fileImage;

   /* If number of command line arguments less than 2, not enough args for
    * filename, list options */
   if (argc < 2) {
      printHelp();

      return EXIT_FAILURE;
   }

   /* Check for output options (-v, -p, etc) */
   while (i < argc - 1) {
      /* If the next string is an option, it's first char is '-' */
      if (argv[i][0] != '-') {
         /* No options specified, move on to set fileImageName */
         break;
      }
      else {
         /* Else grab the flags */
         switch(argv[i][1]) {
         case 'p':
            /* select partition */

            /* Grab which partition to look at */
            whichPartition = atoi(argv[++i]);
            break;
         case 's':
            /* select substitution */

            /* Grab which subpartition to look at */
            whichSubPartition = atoi(argv[++i]);
            break;
         case 'h':
            /* print help */
            printHelp();
            break;
         case 'v':
            /* increase verbosity level */
            verbose = TRUE;
            break;
         default:
            fprintf(stderr, "Invalid flag entered\n");
            break;
         }
      }

      i++;
   }
    
   /* i should be pointing to last argument at this point, if it
    * isnt, that means this command has a path specified */
   if (i != argc - 1) {
      /* malloc space for file name because we'll have to concatenate it */
      /* add the 1 at the end to leave space for the NULL and the '/' */
      
      fileName = malloc(sizeof(char) * FILENAME_LENGTH);
      
      /* Check if filename has a leading '/' if not, add it */
      //if (argv[argc - 1][0] == '/') {
         //sprintf(fileName, "%s%s", argv[argc - 1], argv[i]);
         
         pathName = argv[argc - 1];
         fileName = argv[i];
/*
      }
      else {
         sprintf(fileName, "%s/%s", argv[argc - 1], argv[i]);
      }*/
   }
   else {
      fileName = argv[i];
   }

   /* Open file image for reading */
   fileImage = fopen(fileName, "r");

   /* Checking if file was opened */
   if (fileImage == NULL) {
      fprintf(stderr, "Unable to open disk image \"%s\".\n", argv[i]);

      return EXIT_FAILURE;
   }
   
   /* Error message handling */
   if ((errorMessage = 
            initFileSystem(fileImage, whichPartition, 
               whichSubPartition, pathName))) {
         if (errorMessage == INVALID_PARTITION) {
            fprintf(stderr, "Unable to open disk image \"%s\".\n", fileName);
         }
         else if (errorMessage == NO_FILE_FOUND) {
            fprintf(stderr, "%s: File not found.\n", pathName);
         }
         else if (errorMessage != BAD_MAGIC_NUMBER) {
            fprintf(stderr, "An error occured when attempting to initialize "
                  "the file system\n");
         }

      return EXIT_FAILURE;
   }

   if (verbose) {
      printVerbose(partitionTable, subPartitionTable, superBlock, node);
   }

   printDirectories(fileImage, whichPartition, whichSubPartition, pathName);
   fclose(fileImage);
   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage, int whichPartition, 
      int whichSubPartition, char *pathName) {
   int i;
   uint8_t bootSectValidation510, bootSectValidation511;
   
   /* If a partitition was specified, check the partition table for validity */
   if (whichPartition >= 0) {

      fseek(fileImage, BOOT_SECTOR_BYTE_510, SEEK_SET);
      fread(&bootSectValidation510, sizeof(uint8_t), 1, fileImage);
      fread(&bootSectValidation511, sizeof(uint8_t), 1, fileImage);

      if (bootSectValidation510 != BOOT_SECTOR_BYTE_510_VAL ||
            bootSectValidation511 != BOOT_SECTOR_BYTE_511_VAL) {
         fprintf(stderr, "Partition table does not contain a "
               "valid signature\n");

         return INVALID_PARTITION;
      }
      
      /* Seek to partition sector and read the table */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_SET);

      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         partitionTable[i] = malloc(sizeof(struct partitionEntry));

         fread(partitionTable[i], sizeof(struct partitionEntry), 1,
               fileImage);
         
         
      }
      
      /* Check if the partition table is valid before proceeding */
      if (partitionTable[whichPartition]->type != MINIX_PARTITION_TYPE) {
         fprintf(stderr, "Not a Minix partition.\n");

         return INVALID_PARTITION;
      }

   }
   
   /* Search for sub partiton, if any */
   if (whichSubPartition >= 0) {
      /* Seek to the sector that lFirst of the specified partition points to */
      fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition]->lFirst, 
            SEEK_SET);
      
      fseek(fileImage, BOOT_SECTOR_BYTE_510, SEEK_CUR);
      fread(&bootSectValidation510, sizeof(uint8_t), 1, fileImage);
      fread(&bootSectValidation511, sizeof(uint8_t), 1, fileImage);

      if (bootSectValidation510 != BOOT_SECTOR_BYTE_510_VAL ||
            bootSectValidation511 != BOOT_SECTOR_BYTE_511_VAL) {
         fprintf(stderr, "Partition table does not contain a "
               "valid signature\n");

         return INVALID_PARTITION;
      }
      fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition]->lFirst, 
            SEEK_SET);
      /* Now seek to the partition table of that sub partition */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_CUR);

      /* Read the subpartition table */
      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         subPartitionTable[i] = malloc(sizeof(struct partitionEntry));

         fread(subPartitionTable[i], 
               sizeof(struct partitionEntry), 1, fileImage);

         /* Check if the subpartition table is valid before proceeding */
         //TODO: INCLUDE THIS ERROR CHECKING??
         /*if (i == whichSubPartition && 
           (subPartitionTable[i]->bootind != BOOTABLE_MAGIC_NUM 
            || subPartitionTable[i]->type != MINIX_PARTITION_TYPE)) {
            fprintf(stderr, "Invalid partition table.\n");

            return INVALID_PARTITION;
         }*/
      }
   }
   
   superBlock = getSuperblock(fileImage, whichPartition, partitionTable, 
         whichSubPartition, subPartitionTable);

   if (superBlock->magic != MAGIC_NUMBER) {
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
      
      return BAD_MAGIC_NUMBER;
   }

   //if (whichSubPartition >= 0) {
      node = getInode(fileImage, superBlock, whichPartition, partitionTable,
            whichSubPartition, subPartitionTable, 0);
   /*}
   else {
      node = getInode(fileImage, superBlock, 0, 0);
   }*/

   if (pathName != NULL) {
      if (navigatePath(fileImage, &node, superBlock, partitionTable,
               whichPartition, subPartitionTable, whichSubPartition, 
               pathName)) {
         return NO_FILE_FOUND;
      }
   }

   return EXIT_SUCCESS;
}

void printDirectories(FILE *fileImage, 
                      int whichPartition, 
                      int whichSubPartition,
                      char *pathName) {
   int numZones = (node->size /
            (superBlock->blocksize << superBlock->log_zone_size)) + 1;
   int zoneIdx = 0;
   
   /* If a pathname is specified, navigate to that path */
   if (pathName && !(node->mode & DIRECTORY_MASK)) {
      printf("%s\t%u %s\n", getPermissions(node->mode), 
         node->size, pathName);
   }
   else {
      /* If no path specified, start at the root */
      if (!pathName)
         printf("/:\n");
      else
         printf("%s:\n", pathName);

      /* Create a temp directory entry to store info */
      struct directoryEntry *dir = malloc(sizeof(struct directoryEntry));
      struct inode *tempNode;
      char *dirName;
      int numDirectories;

      /* Check if the size od data is greater than the amount of one zone, 
       * if it is we're going to need to loop through zones */
      numZones = (node->size / zoneSize(superBlock)) + 1;
      
      if (numZones > DIRECT_ZONES) {
               numZones = DIRECT_ZONES;
            }
      
      while (numZones--) {
         if (numZones > 0) {
            /* numDirectories = entries per zone */
            numDirectories = zoneSize(superBlock) / DIRECTORY_ENTRY_SIZE;

           
         }
         else {
            numDirectories = node->size / DIRECTORY_ENTRY_SIZE;

            if ((numDirectories * DIRECTORY_ENTRY_SIZE) > 
                  zoneSize(superBlock)) {
               if (node->indirect) {
                  numDirectories = (zoneSize(superBlock) /
                        DIRECTORY_ENTRY_SIZE);
               }
               else {
                  numDirectories %= DIRECTORY_ENTRY_SIZE;
               }
            }
         }

         /* Set file pointer to past the partitions (if any) */
         seekPastPartitions(fileImage, partitionTable, whichPartition,
            subPartitionTable, whichSubPartition); 
         
         /* Navigate to data zone */
         if (pathName || zoneIdx) {
            fseek(fileImage, node->zone[zoneIdx] * 
              zoneSize(superBlock), SEEK_CUR);

         }
         else {
            fseek(fileImage, superBlock->firstdata * zoneSize(superBlock), 
                  SEEK_CUR);
         }

         while(numDirectories--) {
            fread(dir, sizeof(struct directoryEntry), 1, fileImage);
            dirName = (char*)dir->filename;
            if (dir->inode != 0) {
               tempNode = getInode(fileImage, superBlock, whichPartition,
                        partitionTable, whichSubPartition, subPartitionTable,
                        dir->inode);
               
               printf("%s\t%u %s\n", getPermissions(tempNode->mode), 
                     tempNode->size, dir->filename);

               free(tempNode);
            }
         }

      zoneIdx++;
      }
      
      if (node->indirect) {
         getIndirectBlock(fileImage, node, superBlock,
            partitionTable, whichPartition, subPartitionTable,
            whichSubPartition, NULL);

      }
   }
}
