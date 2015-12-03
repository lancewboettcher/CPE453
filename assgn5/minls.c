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
#define BAD_MAGIC_NUMBER -1
#define INVALID_PARTITION -2
#define NO_FILE_FOUND -3

/* Global Variables */
struct superblock *superBlock = NULL;
struct partitionEntry *partitionTable = NULL;
struct partitionEntry *subPartitionTable = NULL;
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
      pathName = argv[argc - 1];
   }
   
   /* At this point i should be pointing to the argument immediately after
    * the option flags. This is the name of the file image to read. */
   fileName = argv[i];
   
   /* Open file image for reading */
   fileImage = fopen(fileName, "r");

   /* Checking if file was opened */
   if (fileImage == NULL) {
      fprintf(stderr, "Unable to open disk image \"%s\".\n", argv[i]);

      return EXIT_FAILURE;
   }
   
   /* Initialize the file system by setting all of the structues (superblock, 
    * partition tables, etc). If the function has a return value that means an
    * error has occured, handle it accordingly. */
   if ((errorMessage = initFileSystem(fileImage, whichPartition, 
               whichSubPartition, pathName))) {
         if (errorMessage == INVALID_PARTITION) {
            /* The partition table specified is not valid. This can be due to 
             * a bad magic number, not a minix partition, etc */
            fprintf(stderr, "Unable to open disk image \"%s\".\n", fileName);
         }
         else if (errorMessage == NO_FILE_FOUND) {
            /* If a certain file path was specified to list but the path
             * is not valid or the file does not exist, return no file found */
            fprintf(stderr, "%s: File not found.\n", pathName);
         }
         else if (errorMessage != BAD_MAGIC_NUMBER) {
            /* For all other errors (except when the superblock contains a 
             * bad magic number) print out that an error occured when 
             * attempting to initialize the file system. Don't print anything
             * when a superblock has a bad magic number in order to match 
             * the output of the demo minls */
            fprintf(stderr, "An error occured when attempting to initialize "
                  "the file system\n");
         }

      return EXIT_FAILURE;
   }

   /* If the verbose flag was specifed this boolean will be true and the
    * printVerbose function will print contents of superblock */
   if (verbose) {
      printVerbose(partitionTable, subPartitionTable, superBlock, node);
   }

   /* At this point the file system is initialized and read, the pathName (if
    * any) was found and the global 'node' variable is pointing to it. Now
    * can print the contents of the directory */
   printDirectories(fileImage, whichPartition, whichSubPartition, pathName);
   
   fclose(fileImage);
   free(superBlock);
   free(node);
   free(partitionTable);
   free(subPartitionTable);

   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage, int whichPartition, 
      int whichSubPartition, char *pathName) {
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

   /* Grab superBlock from fileImage and set the global variable */
   superBlock = getSuperblock(fileImage, whichPartition, partitionTable, 
         whichSubPartition, subPartitionTable);

   /* Check if superBlock is valid minix superblock */
   if (superBlock->magic != MAGIC_NUMBER) {
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
      
      return BAD_MAGIC_NUMBER;
   }

   /* Grab the root inode from fileImage and set the global variable.
    * It's the first inode of the block, thus offset (last parameter)
    * is 0. */
   node = getInode(fileImage, superBlock, whichPartition, partitionTable,
            whichSubPartition, subPartitionTable, 0);

   /* If optional pathname specified, search through all of the directory
    * entried for it and navigate through the file image via inodes. This
    * function also sets the global 'node' variable to the inode that the 
    * pathname specifies */
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
   int numZones;
   int zoneIdx = 0;
   
   if (pathName && !(node->mode & DIRECTORY_MASK)) {
      /* If a pathname is specified and it points to a single file (not a 
       * directory) only need to list that single file and its permissions */
      printf("%s\t%u %s\n", getPermissions(node->mode), 
         node->size, pathName);
   }
   else {
      if (!pathName) {
         /* If no path specified, start at the root */
         printf("/:\n");
      }
      else {
         /* Print the path to the directory that will be listed */
         printf("%s:\n", pathName);
      }

      /* Create a temp directory entry to store info */
      struct directoryEntry *dir = malloc(sizeof(struct directoryEntry));
      struct inode *tempNode;
      int numEntries;

      /* Check if the size od data is greater than the amount of one zone, 
       * if it is we're going to need to loop through zones */
      numZones = getNumberOfZones(fileImage, superBlock, node);
      
      if (numZones > DIRECT_ZONES) {
         /* If number of zones is greater than the amount the direct zones can
          * hole, then set numZones to the max amount of direct zones, (because
          * you can't iterate through more than that until getting to indirect
          * zones). Else it will be set to however many zones it needs */
         numZones = DIRECT_ZONES;
      }
      
      while (numZones--) {
         if (numZones > 0) {
            /* if numZones left is > 0, that means this is not the last zone
             * to read, so numEntries to read is set to the max amount that 
             * can be read in a zone */
            numEntries = entriesPerZone(superBlock);
         }
         else {
            /* Last direct node to read*/

            /* Get total number of entries to read for a node of this size */
            numEntries = node->size / DIRECTORY_ENTRY_SIZE;

            if ((numEntries * DIRECTORY_ENTRY_SIZE) > zoneSize(superBlock)) {
               /* If the amount of space needed for this number of entries
                * is greater than one zone (the last direct zone specified) */
               if (node->indirect) {
                  /* If the node has more data in an indirect zone, then set
                   * the amount of entries to read to the max amount (since
                   * this is not the last data to be read). */
                  numEntries = entriesPerZone(superBlock);
               }
               else {
                  /* No more zones to read, divide the total numEntires read
                   * by the size of a directory entry, should be left over with
                   * the amount of entries that still need to be read */
                  numEntries %= DIRECTORY_ENTRY_SIZE;
               }
            }
         }

         /* Set file pointer to past the partitions (if any) */
         seekPastPartitions(fileImage, partitionTable, whichPartition,
            subPartitionTable, whichSubPartition); 
         
         if (pathName || zoneIdx) {
            /* Navigate to data zone */
            fseek(fileImage, node->zone[zoneIdx] * zoneSize(superBlock), 
                  SEEK_CUR);
         }
         else {
            /* Else seek from beginning of file */
            fseek(fileImage, superBlock->firstdata * zoneSize(superBlock), 
                  SEEK_CUR);
         }

         /* Read the amount od directory entries specified */
         while(numEntries--) {
            fread(dir, sizeof(struct directoryEntry), 1, fileImage);
            
            /* If the inode of the directory is not zero, the entry is not
             * marked as deleted and is valid. */
            if (dir->inode != 0) {
               /* Grab the inode specified and print the contents */
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
      
      /* If the node has data in an direct zone, grab the indirect zone,
       * iterate through all of the zones specifed, and print them out. 
       * Because we want to list all files and not just return the one 
       * specified, the fileToGet (last parameter) is NULL */
      if (node->indirect) {
         getIndirectBlock(fileImage, node, superBlock,
            partitionTable, whichPartition, subPartitionTable,
            whichSubPartition, NULL);

      }
   }
}
