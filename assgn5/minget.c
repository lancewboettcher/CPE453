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
FILE *destPath = NULL;
uint32_t *indirectZoneList = NULL;

/* Function Prototypes */
void getFile(FILE*, int, int, char*);
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
      
      fileName = malloc(sizeof(char) * FILENAME_LENGTH);
      
      /* if there are 3 arguments after the flags end, that means the 
       * very last argument is a file destination  path name */
      if ((argc-1) - i >= 2) {
         pathName = argv[i + 1];
         fileName = argv[i];
         destPath = fopen(argv[argc - 1], "w");

         if (destPath == NULL) {
            fprintf(stderr, "There was an issue opening the destination "
                  "path file\n");
            return EXIT_FAILURE;
         }
      }
      else {
         /* No optional file path given, default to stdout */
         /* Check if filename has a leading '/' if not, add it */
         
            pathName = argv[argc - 1];
            fileName = argv[i];


      }
      
      
   }
   else {
      fileName = argv[i];
   }

   /* Open file image for reading */
   fileImage = fopen(fileName, "r");

   /* if destination path is still null, that means no destination path
    * specified, default to stdout */
   if (destPath == NULL) {
      destPath = stdout;
   }

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
 
   if ((node->mode & FILE_TYPE_MASK) != REGULAR_FILE_MASK) {
         fprintf(destPath, "%s: Not a regular file.\n", pathName);
         
         return EXIT_FAILURE;
   }

   getFile(fileImage, whichPartition, whichSubPartition, pathName);
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

   if (pathName != NULL) {
      if (navigatePath(fileImage, &node, superBlock, partitionTable,
               whichPartition, subPartitionTable, whichSubPartition, 
               pathName)) {
         return NO_FILE_FOUND;
      }
   }

   return EXIT_SUCCESS;
}

int getNumberOfZones(FILE* fileImage) {
   int numZones;
   
   if (node->size % zoneSize(superBlock)) {
      numZones = (node->size / zoneSize(superBlock)) + 1;
   }
   else {
      numZones = (node->size / zoneSize(superBlock));
   }
   return numZones;
}

void getFile(FILE *fileImage, 
                      int whichPartition, 
                      int whichSubPartition,
                      char *pathName) {
   char fileOutput[zoneSize(superBlock)];
   int numberOfZones = getNumberOfZones(fileImage);
   int zonesToRead, zoneIdx = 0, amountToRead, i;

   if (numberOfZones > DIRECT_ZONES) {
      zonesToRead = DIRECT_ZONES;
   }
   else {
      zonesToRead = numberOfZones;
   }
   
   if (zonesToRead == 1 ) {
      amountToRead = node->size; 
   }
   else {
      amountToRead = zoneSize(superBlock);
   }

   while (zonesToRead--) {
      if (!zonesToRead && !node->indirect) {
         amountToRead = node->size % zoneSize(superBlock);
         
         if (!amountToRead) {
            amountToRead = zoneSize(superBlock);
         }
      }
      
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);

      /* Navigate<S-F10> to data zone */
      fseek(fileImage, node->zone[zoneIdx] * zoneSize(superBlock),
         SEEK_CUR);

      fread(fileOutput, amountToRead, 1, fileImage);
      fwrite(fileOutput, 1, amountToRead, destPath);

      zoneIdx++;
   }
   if (node->indirect) {
      if (!node->two_indirect) {
         zonesToRead = numberOfZones - DIRECT_ZONES; 
      }
      else {
         fprintf(stderr, "INSERT MAX INDIRECT ZONES\n");
      }
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);
      
      fseek(fileImage, node->indirect * zoneSize(superBlock), SEEK_CUR);
      
      uint32_t *indirectZones[zonesToRead];
      for (i = 0; i < zonesToRead; i++) {
         indirectZones[i] = malloc(sizeof(uint32_t));
         fread(indirectZones[i], sizeof(uint32_t), 1, fileImage);
      }

      zoneIdx = 0;
      
      if (zonesToRead == 1) {
         amountToRead = node->size % zoneSize(superBlock);
         
         if (!amountToRead) {
            amountToRead = zoneSize(superBlock);
         }

      }
      else {
         amountToRead = zoneSize(superBlock);
      }

      while(zonesToRead--) {
         if (!zonesToRead && !node->two_indirect) {
            amountToRead = node->size % zoneSize(superBlock);
         
            
            if (!amountToRead) {
               amountToRead = zoneSize(superBlock);
            }
         }

         seekPastPartitions(fileImage, partitionTable, whichPartition,
            subPartitionTable, whichSubPartition);
      
         fseek(fileImage, *indirectZones[zoneIdx] * zoneSize(superBlock), 
               SEEK_CUR);
         

         fread(fileOutput, amountToRead, 1, fileImage);
         fwrite(fileOutput, 1, amountToRead, destPath);

         zoneIdx++;
      }
   }

}
