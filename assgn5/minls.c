//TODO: read path from command line
//TODO: add 4 primary partition tables
//TODO: add subpartition table functionality
#include "filesystem.h"

#define TRUE 1
#define FALSE 0
#define BYTESIZE 8
#define BAD_MAGIC_NUMBER -1
#define INVALID_PARTITION -2

/* Global Variables */
struct superblock *superBlock = NULL;
struct partitionEntry *partitionTable[4] = {NULL};
struct partitionEntry *subPartitionTable[4] = {NULL};


/* Function Prototypes */
void printHelp(void);
void printVerbose(void);
int initFileSystem(FILE *, int, int);

int main (int argc, char *argv[]) {
   int i = 1;
   int errorMessage;
   int verbose = FALSE;
   int whichPartition = -1, whichSubPartition = -1;
   char *fileName;
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

   /* i should be pointing to last argument at this point */
   fileName = argv[i];

   /* Open file image for reading */
   fileImage = fopen(fileName, "r");

   /* Checking if file was opened */
   if (fileImage == NULL) {
      fprintf(stderr, "Unable to open file specified\n");

      return EXIT_FAILURE;
   }
   
   /* Error message handling */
   if ((errorMessage = 
            initFileSystem(fileImage, whichPartition, whichSubPartition))) {
         if (errorMessage == INVALID_PARTITION) {
            fprintf(stderr, "Unable to open disk image \"%s\".\n", fileName);
         }
         else if (errorMessage != BAD_MAGIC_NUMBER) {
            fprintf(stderr, "An error occured when attempting to initialize "
                  "the file system\n");
         }

      return EXIT_FAILURE;
   }

   if (verbose) {
      printVerbose();
   }

   fclose(fileImage);
   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage, int whichPartition, 
      int whichSubPartition) {
   //TODO: CHECK THE BOOT IMAGE (See comment below)
   /* First check the boot block for magic number */

   int i;

   /* Initialize the blocks */
   superBlock = malloc(sizeof(struct superblock));   
   
   /* If a partitition was specified, check the partition table for validity */
   if (whichPartition >= 0) {
      uint8_t bootSectValidation510, bootSectValidation511;

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

      for (i=0; i<NUM_PRIMARY_PARTITONS; i++) {
         partitionTable[i] = malloc(sizeof(struct partitionEntry));

         fread(partitionTable[i], sizeof(struct partitionEntry), 1,
               fileImage);

         /* Check if the partition table is valid before proceeding */
         if (i == whichPartition && 
               (partitionTable[i]->bootind != BOOTABLE_MAGIC_NUM 
            || partitionTable[i]->type != MINIX_PARTITION_TYPE)) {
            fprintf(stderr, "Invalid partition table.\n");

            return INVALID_PARTITION;
         }
      }
   }

   /* Search for sub partiton, if any */
   if (whichSubPartition >= 0) {
      /* Seek to the sector that lFirst of the specified partition points to */
      fseek(fileImage, SECTOR_SIZE * partitionTable[whichPartition]->lFirst, 
            SEEK_SET);

      /* Now seek to the partition table of that sub partition */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_CUR);

      /* Read the subpartition table */
      for (i=0; i<NUM_PRIMARY_PARTITONS; i++) {
         subPartitionTable[i] = malloc(sizeof(struct partitionEntry));

         fread(subPartitionTable[i], 
               sizeof(struct partitionEntry), 1, fileImage);

         /* Check if the subpartition table is valid before proceeding */
         //TODO: INCLUDE THIS ERROR CHECKING??
         /*if (i == whichSubPartition && 
          * (subPartitionTable[i]->bootind != BOOTABLE_MAGIC_NUM 
            || subPartitionTable[i]->type != MINIX_PARTITION_TYPE)) {
            fprintf(stderr, "Invalid partition table.\n");

            return INVALID_PARTITION;
         }*/
      }
   }

   /* If no partiton table, seek to the next block (boot block is 2 sectors),
    * else use the subpartition specified and seek to that boot block */
   if (whichSubPartition >= 0) {
      fseek(fileImage, SECTOR_SIZE * 
            subPartitionTable[whichSubPartition]->lFirst, SEEK_SET);
   }
   else {
      fseek(fileImage, SECTOR_SIZE * 2, SEEK_SET);
   }

   fread(superBlock, sizeof(struct superblock), 1, fileImage);

   if (superBlock->magic != MAGIC_NUMBER) {
      printVerbose();
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
      
      return BAD_MAGIC_NUMBER;
   }

   return EXIT_SUCCESS;
}

void printVerbose() {
   int i;

   if (*partitionTable != NULL) {
      printf("\nPartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      "
            "First       Size\n");
      for (i=0; i<NUM_PRIMARY_PARTITONS; i++) {
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
      for (i=0; i<NUM_PRIMARY_PARTITONS; i++) {
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
   /*
   printf("Computed Fields:\n");
   printf("  version       %u\n", 99999);
   printf("  firstImap       %u\n", 99999);
   printf("  firstZmap       %u\n", 99999);
   printf("  firstIblock       %u\n", 2 + 
         superBlock->i_blocks + superBlock->z_blocks);
   printf("  zonesize       %u\n", 
         superBlock->blocksize << superBlock->log_zone_size);
   printf("  ptrs_per_zone       %u\n", 99999);
   printf("  ino_per_block       %lu\n", 
         superBlock->blocksize / sizeof(struct inode));
   printf("  wrongended       %u\n", 999999);
   printf("  fileent_size       %u\n", 99999);
   printf("  max_filename       %u\n", 99999);
   printf("  ent_per_zone       %u\n", 99999);*/
}

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
