#ifndef FILESYSTEM_H
#include "filesystem.h"
#endif

#ifndef INODE_H
#include "inode.h"
#endif

#ifndef SUPERBLOCK_H
#include "superblock.h"
#endif 

#ifndef PARTITION_H
#include "partition.h"
#endif 

#define TRUE 1
#define FALSE 0
#define BYTESIZE 8

/* Global Variables */
struct superblock *superBlock = NULL;
struct partitionEntry *partitionTable = NULL;
struct partitionEntry *subPartitionTable = NULL;
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
   /* If a partitition was specified, check the partition table for validity */
   if (getPartitionTable(fileImage, &partitionTable, whichPartition, 0)) {
         return INVALID_PARTITION;
   }
   
   if (whichSubPartition >= 0) {
      /* Check the bytes 510 and 511 of the partition table for the magic 
       * numbers */
      if (checkPartitionMagic(fileImage, partitionTable, whichPartition)) {
         /* If there was a return value, that means an error occured and the
          * partition is invalid */
         return INVALID_PARTITION;
      }
      
      /* Search for sub partiton, if any */
      if (getPartitionTable(fileImage, &subPartitionTable, whichSubPartition,
               partitionTable[whichPartition].lFirst)) {
            return INVALID_PARTITION;
      }
   }

   superBlock = getSuperblock(fileImage, whichPartition, partitionTable, 
         whichSubPartition, subPartitionTable);

   /* Check if the magic number in the superblock equals the magic number
    * for Minix 3 file systems */
   if (superBlock->magic != MAGIC_NUMBER) {
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
                        
      return BAD_MAGIC_NUMBER;
   }
   
   /* Get the first inode of the file system (which is why the last argument,
    * the inode offset, is 0) */
   node = getInode(fileImage, superBlock, whichPartition, partitionTable,
                     whichSubPartition, subPartitionTable, 0);
   
   /* If a pathName is specified, navigate through the file system for the
      * file specified */
   if (pathName != NULL) {
      if (navigatePath(fileImage, &node, superBlock, partitionTable,
               whichPartition, subPartitionTable, whichSubPartition, 
               pathName)) {
         /* If the navigatePath function has a return value, that means 
            * the file was not found */
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
         /* If there is only one zone, only have to read the remainder of 
          * data left after reading all of the rest of the zones with a 
          * full zone size of data */
         amountToWrite = node->size % zoneSize(superBlock);
         
         if (!amountToWrite) {
            /* If the remainder == 0, that means the amount of data to read
             * is a multiple of 4096. In this case, the whole zone is full
             * and needs to be written to the file */
            amountToWrite = zoneSize(superBlock);
         }
      }
      else {
         /* If there is more than one zone, need to write the full zoneSize
          * worth of data to the output file */
         amountToWrite = zoneSize(superBlock);
      }

      /* Loop through the amount of zones and write data to the file */
      while(amountOfZones--) {
         if (!amountOfZones && !node->two_indirect) {
            /* If this is the last zone to be read and there are no double
             * indirect zones after this indirect zone, then all that's left
             * to write is the remainder after all previous zones are filled */
            amountToWrite = node->size % zoneSize(superBlock);
            
            if (!amountToWrite) {
               /* If remainder 0, read full zone */
               amountToWrite = zoneSize(superBlock);
            }
         }

         if (*indirectZones[zoneIdx]) {
            /* Only seek to the zone and read/write the data if it has a 
             * valid (non-zero) zone value */
            seekPastPartitions(fileImage, partitionTable, whichPartition,
               subPartitionTable, whichSubPartition);
         
            fseek(fileImage, *indirectZones[zoneIdx] * zoneSize(superBlock), 
                  SEEK_CUR);
            
            /* read in data from file image and write it to file specified */
            fread(output, amountToWrite, 1, fileImage);
            fwrite(output, 1, amountToWrite, destPath);
            
            /* Keep track of how much data has been read/written */
            amountAlreadyRead += amountToWrite;
         }
         else {
            /* If zone == 0, the zone is a whole. Write a zone's worth of NULLs
             * to the desired output file */
            memset(output, '\0', zoneSize(superBlock));
            fwrite(output, 1, zoneSize(superBlock), destPath);

            /* Keep track of how much data has been read/written */
            amountAlreadyRead += zoneSize(superBlock);
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
      /* If number of zones is greater than the amount the direct zones can
       * hole, then set numZones to the max amount of direct zones, (because
       * you can't iterate through more than that until getting to indirect
       * zones). */
       zonesToRead = DIRECT_ZONES;
   }
   else {
      /* Else it will be set to however many zones it needs */
      zonesToRead = numberOfZones;
   }
   
   if (zonesToRead == 1 ) {
      /* If only one zone to read, then read in full node size in that one
       * zone */
      amountToRead = node->size; 
   }
   else {
      /* If more than one zone to read, read in the max amount that a zone
       * can hold */
      amountToRead = zoneSize(superBlock);
   }
    
   /* Loop through zones */
   while (zonesToRead--) {
      if (!zonesToRead && !node->indirect) {
         /* If last zone and no indirect zone specified, nead in the 
          * remainder of the node size */
         amountToRead = node->size % zoneSize(superBlock);
         
         if (!amountToRead) {
            /* If reaminder 0, read full zone */
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

         /* Read/write data and increment amount of data read */
         fread(fileOutput, amountToRead, 1, fileImage);
         fwrite(fileOutput, 1, amountToRead, destPath);

         amountAlreadyRead += amountToRead;
      }
      else {
         /* Encountered hole. Write out zone's worth of null values and
          * increment amount read */
         memset(fileOutput, '\0', zoneSize(superBlock));
         fwrite(fileOutput, 1, zoneSize(superBlock), destPath);

         amountAlreadyRead += zoneSize(superBlock);
      }
      zoneIdx++;
   }
   
   if (node->indirect) {
      /* If there is an indirect zone specified */
      if (!node->two_indirect) {
         /* If no indirect zone specified, amount to read is just the 
          * amount of zones left over after the 7 direct zones are read */
         zonesToRead = numberOfZones - DIRECT_ZONES; 
      }
      else {
         /* Else jsut read the maz amount of zones that can be in an 
          * indirect zone since there is still data to read after */
         zonesToRead = pointersPerZone(superBlock);
      }

      /* Read the indirect zones */
      readIndirectZones(fileImage, whichPartition, whichSubPartition,
            node->indirect, zonesToRead);
   }
   
   
   if (node->two_indirect) {
      /* If the node has a couble indirect zone */
      char indirectZoneHole[zoneSize(superBlock) * 
         pointersPerZone(superBlock)];
      
      if (!node->indirect) {
         /* If no previous indirect node, the whole indirect zone was a hole.
          * Write in the null values to the output file */
         memset(indirectZoneHole, '\0', zoneSize(superBlock) * 
               pointersPerZone(superBlock));
         fwrite(indirectZoneHole, 1, sizeof(indirectZoneHole), destPath);
         
         amountAlreadyRead += sizeof(indirectZoneHole);
      }

      /* Calculate amount od data left to read */
      int amountLeft = node->size - amountAlreadyRead;
      int indirZonesToRead, doubleIndirZonesToRead;
      
      /* Calculate how many zones to read based on amount left. If there is 
       * a remainder, the round the number of zones up by 1. */
      if (amountLeft % zoneSize(superBlock)) {
         zonesToRead = (amountLeft / zoneSize(superBlock)) + 1;
      }
      else {
         zonesToRead = amountLeft / zoneSize(superBlock);
      }
               
      if (zonesToRead <= pointersPerZone(superBlock)) {
         /* If the amount of zones to read can be held in a single double 
          * indirect zone, then set that to one, and read in the amount of 
          * indirect zones necessary. */
         indirZonesToRead = zonesToRead;
         doubleIndirZonesToRead = 1;
      }
      else {
         /* Else need to read multiple double indirect zones */
         /* Calculate how many zones to read based on amount left. If there is 
          * a remainder, the round the number of double indir zones up by 1. */
         if (zonesToRead % pointersPerZone(superBlock)) {
            doubleIndirZonesToRead = (zonesToRead / 
                  pointersPerZone(superBlock)) + 1;
         }
         else {
            doubleIndirZonesToRead = (zonesToRead / 
                  pointersPerZone(superBlock));
         }
         
         /* Set num of indirect zones to read to the max amount */
         indirZonesToRead = pointersPerZone(superBlock);
      }

      /* Set file pointer to past the partitions (if any) */
      seekPastPartitions(fileImage, partitionTable, whichPartition,
      subPartitionTable, whichSubPartition);
      
      /* Seek to the double indirect zone */
      fseek(fileImage, node->two_indirect * zoneSize(superBlock), SEEK_CUR);
   
      uint32_t *doubleIndirectZones[doubleIndirZonesToRead];
       
      /* Read in all of the indirect zone numbers */
      for (i = 0; i < doubleIndirZonesToRead; i++) {
         doubleIndirectZones[i] = malloc(sizeof(uint32_t));
         fread(doubleIndirectZones[i], sizeof(uint32_t), 1, fileImage);
      }
      
      /* Loop through the double indirect zones to read */
      for (i = 0; i < doubleIndirZonesToRead; i++) {
         /* Calculate amount left to read */
         amountLeft = node->size - amountAlreadyRead;
         
         /* Calculate how many zones to read based on amount left. If there is 
          * a remainder, the round the number of zones up by 1. */
         if (amountLeft % zoneSize(superBlock)) {
            zonesToRead = (amountLeft / zoneSize(superBlock)) + 1;
         }
         else {
            zonesToRead = amountLeft / zoneSize(superBlock);
         }
   
         if (zonesToRead <= pointersPerZone(superBlock)) {
            /* if you can read in the rest of the data in a single indirect
             * sone, set zones to read to that value */
            indirZonesToRead = zonesToRead;
         }
         else {
            /* Else set to max amount */
            indirZonesToRead = pointersPerZone(superBlock);
         }
   
         if  (*doubleIndirectZones[i] == 0) {
            /* if the zone is 0, it's a hole. Fill it with nulls */
            memset(indirectZoneHole, '\0', zoneSize(superBlock) * 
                  pointersPerZone(superBlock));
            fwrite(indirectZoneHole, 1, sizeof(indirectZoneHole), destPath);

            amountAlreadyRead += sizeof(indirectZoneHole);
         }
         else {
            /* Else it's a valid zone. read in the indirect zone */
            readIndirectZones(fileImage, whichPartition, whichSubPartition,
            *doubleIndirectZones[i], indirZonesToRead);
         }
      }

   }
}
