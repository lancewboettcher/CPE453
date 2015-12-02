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

/* Global Variables */
struct superblock *superBlock = NULL;
struct partitionEntry *partitionTable[4] = {NULL};
struct partitionEntry *subPartitionTable[4] = {NULL};
struct inode *node = NULL;
FILE *destPath = NULL;
uint32_t *indirectZoneList = NULL;
uint32_t amountAlreadyRead = 0;

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
            /* select partition number */
            /* Grab which partition to look at */
            whichPartition = atoi(argv[++i]);
            break;
         case 's':
            /* select subpartition number*/
            /* Grab which subpartition to look at */
            whichSubPartition = atoi(argv[++i]);
            break;
         case 'h':
            /* print help */
            printHelp();
            break;
         case 'v':
            /* set the verbose boolean to TRUE to print after initialization */
            verbose = TRUE;
            break;
         default:
            fprintf(stderr, "Invalid flag entered\n");
            break;
         }
      }

      i++;
   }
    
   /* i should be pointing to the first argument after the flags at this 
    * point, which is the fileName where the file image its located */
   fileName = malloc(sizeof(char) * FILENAME_LENGTH);
   fileName = argv[i];

   /* If i is not pointing at the last argument, then that means thatr there
    * is the optional file path specified */
   if (i != argc - 1) {
      /* If there are 3 arguments after the flags end, that means the 
       * very last argument is a file destination  path name */
      if ((argc-1) - i >= 2) {
         pathName = argv[i + 1];
         destPath = fopen(argv[argc - 1], "w");

         if (destPath == NULL) {
            fprintf(stderr, "There was an issue opening the destination "
                  "path file\n");
            return EXIT_FAILURE;
         }
      }
      else {
         /* No optional file path given, default to stdout */
         pathName = argv[argc - 1];
      }
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
   
   /* Initialize the file system and handle any errors that may have occured */
   if ((errorMessage = 
            initFileSystem(fileImage, whichPartition, 
               whichSubPartition, pathName))) {
         if (errorMessage == INVALID_PARTITION) {
            /* Partition specified does not exist or is invalid */
            fprintf(stderr, "Unable to open disk image \"%s\".\n", fileName);
         }
         else if (errorMessage == NO_FILE_FOUND) {
            /* Unable to find the file specified by the path name after 
             * attempting to navigate the fileImage */
            fprintf(stderr, "%s: File not found.\n", pathName);
         }
      
      /* For any other errors that occur, no additional print message is
       * required, just need to return EXIT_FAILURE */
      return EXIT_FAILURE;
   }

   /* If command line specifies -v, print verbose */
   if (verbose) {
      printVerbose(partitionTable, subPartitionTable, superBlock, node);
   }
   
   /* At this point the global "node" variable is pointing to the inode
    * specified by the directory entry of the destination path, if this
    * inode's mode is NOT a regular file, unable to 'get' file, print error
    * and exit program. */
   if ((node->mode & FILE_TYPE_MASK) != REGULAR_FILE_MASK) {
         fprintf(destPath, "%s: Not a regular file.\n", pathName);
         
         return EXIT_FAILURE;
   }

   /* File system initializes, inode points to a regular file and the file 
    * exists, now we can print the file to the destination path */ 
   getFile(fileImage, whichPartition, whichSubPartition, pathName);
   
   fclose(fileImage);
   
   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage,
                   int whichPartition, 
                   int whichSubPartition, 
                   char *pathName) {
   int i;
   
   /* If a partitition was specified, check the partition table for validity */
   if (whichPartition >= 0) {
      /* Seek to partition sector and read the table */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_SET);

      for (i=0; i<NUM_PRIMARY_PARTITIONS; i++) {
         partitionTable[i] = malloc(sizeof(struct partitionEntry));

         fread(partitionTable[i], sizeof(struct partitionEntry), 1,
               fileImage);
      }
      
      if (partitionTable[whichPartition]->type != MINIX_PARTITION_TYPE) {
         fprintf(stderr, "Not a Minix partition\n");

         return INVALID_PARTITION;
      }
   }

   /* Search for sub partiton, if any */
   if (whichSubPartition >= 0) {
      /* Check the bytes 510 and 511 of the partition table for the magic 
       * numbers */
      if (checkPartitionMagic(fileImage, partitionTable, whichPartition)) {
         /* If there was a return value, that means an error occured and the
          * partition is invalid */
         return INVALID_PARTITION;
      }


      /* Seek to the first absolute sector of the partition specified */
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

   /* Get the superblock of the file image */
   superBlock = getSuperblock(fileImage, whichPartition, partitionTable, 
         whichSubPartition, subPartitionTable);

   /* Check if the magic number in the superblock equals the magic number
    * for Minix 3 file systems */
   if (superBlock->magic != MAGIC_NUMBER) {
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
      
      return BAD_MAGIC_NUMBER;
   }

   /* Get the first inode of the file system (which is why the last argumennt, 
    * the inode offset, is 0) */
   node = getInode(fileImage, superBlock, whichPartition, partitionTable,
            whichSubPartition, subPartitionTable, 0);

   /* If a pathName is specified, navigate through the file system for the
    * file specified */
   if (pathName != NULL) {
      if (navigatePath(fileImage, &node, superBlock, partitionTable,
               whichPartition, subPartitionTable, whichSubPartition, 
               pathName)) {
         /* If the navigatePath function has a return value, that means the
          * file was not found */
         return NO_FILE_FOUND;
      }
   }

   return EXIT_SUCCESS;
}

void readIndirectZones(FILE * fileImage,
                       int whichPartition,
                       int whichSubPartition,
                       uint32_t zoneNumber,
                       uint32_t amountOfZones) {
      int i, amountToWrite, zoneIdx = 0;
      char output[zoneSize(superBlock)];
      uint32_t *indirectZones[amountOfZones];
      
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);
      
      /* Seek to the zone specified */
      fseek(fileImage, zoneNumber * zoneSize(superBlock), SEEK_CUR);
     
      /* Indirect zones are zones full of zone numbers to other data blocks. So,
       * malloc an array of uint32_t values and read in each one to get
       * the list of zone numbers specified */
      for (i = 0; i < amountOfZones; i++) {
         indirectZones[i] = malloc(sizeof(uint32_t));
         fread(indirectZones[i], sizeof(uint32_t), 1, fileImage);
      }

      if (amountOfZones == 1) {
         amountToWrite = node->size % zoneSize(superBlock);
         
         if (!amountToWrite) {
            amountToWrite = zoneSize(superBlock);
         }

      }
      else {
         amountToWrite = zoneSize(superBlock);
      }

      while(amountOfZones--) {
         if (!amountOfZones && !node->two_indirect) {
            amountToWrite = node->size % zoneSize(superBlock);
            
            if (!amountToWrite) {
               amountToWrite = zoneSize(superBlock);
            }
         }

         if (*indirectZones[zoneIdx]) {
            seekPastPartitions(fileImage, partitionTable, whichPartition,
               subPartitionTable, whichSubPartition);
         
            fseek(fileImage, *indirectZones[zoneIdx] * zoneSize(superBlock), 
                  SEEK_CUR);
            
            
            fread(output, amountToWrite, 1, fileImage);
            fwrite(output, 1, amountToWrite, destPath);
            
            amountAlreadyRead += amountToWrite;
         }
         else {
            memset(output, '\0', zoneSize(superBlock));
            fwrite(output, 1, zoneSize(superBlock), destPath);
         }
         zoneIdx++;
      }

}

void getFile(FILE *fileImage, 
                      int whichPartition, 
                      int whichSubPartition,
                      char *pathName) {
   char fileOutput[zoneSize(superBlock)];
   int numberOfZones = getNumberOfZones(fileImage, superBlock, node);
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
      
      if (node->zone[zoneIdx]) {
         /* Navigate<S-F10> to data zone */
         fseek(fileImage, node->zone[zoneIdx] * zoneSize(superBlock),
            SEEK_CUR);

         fread(fileOutput, amountToRead, 1, fileImage);
         fwrite(fileOutput, 1, amountToRead, destPath);

         amountAlreadyRead += amountToRead;
      }
      else {
         memset(fileOutput, '\0', zoneSize(superBlock));
         fwrite(fileOutput, 1, zoneSize(superBlock), destPath);
      }
      zoneIdx++;
   }
   
   if (node->indirect) {
      if (!node->two_indirect) {
         zonesToRead = numberOfZones - DIRECT_ZONES; 
      }
      else {
         zonesToRead = PTRS_PER_ZONE;
      }

      readIndirectZones(fileImage, whichPartition, whichSubPartition,
            node->indirect, zonesToRead);
   }
   
   if (node->two_indirect) {
      if (!node->indirect) {
         char indirectZoneHole[zoneSize(superBlock) * PTRS_PER_ZONE];
         memset(indirectZoneHole, '\0', zoneSize(superBlock) * PTRS_PER_ZONE);
         fwrite(indirectZoneHole, 1, sizeof(indirectZoneHole), destPath);
      }
      
      zonesToRead = numberOfZones - DIRECT_ZONES - PTRS_PER_ZONE; 
      
      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
         subPartitionTable, whichSubPartition);
      
      fseek(fileImage, node->two_indirect * zoneSize(superBlock), SEEK_CUR);
      
      uint32_t *doubleIndirectZones[zonesToRead];
      
      for (i = 0; i < zonesToRead; i++) {
         doubleIndirectZones[i] = malloc(sizeof(uint32_t));
         fread(doubleIndirectZones[i], sizeof(uint32_t), 1, fileImage);
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
   
      int toRead = zonesToRead;
      for (i = 0; i < zonesToRead; i++) {
         if ( toRead > PTRS_PER_ZONE * zoneSize(superBlock)) {
            toRead = PTRS_PER_ZONE * zoneSize(superBlock);
            return;
         }
         
         if ( *doubleIndirectZones[i] ) {
            readIndirectZones(fileImage, whichPartition, whichSubPartition,
               *doubleIndirectZones[i], toRead);
         }
         else {

         }

      }
   }

}
