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
struct partitionEntry *partitionTable = NULL;

/* Function Prototypes */
void printHelp(void);
void printVerbose(void);
int initFileSystem(FILE *fileImage, int hasPartition);

int main (int argc, char *argv[]) {
   int i = 1;
   int errorMessage;
   int verbose = FALSE, partitionVal = FALSE;
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
            partitionVal = TRUE;
            break;
         case 's':
            /* select substitution */
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
   if ((errorMessage = initFileSystem(fileImage, partitionVal))) {
         if (errorMessage == INVALID_PARTITION) {
            fprintf(stderr, "Unable to open disk image \"%s\".\n", fileName);
         }
         else if (errorMessage != BAD_MAGIC_NUMBER) {
            fprintf(stderr, "An error occured when attempting to initialize the "
                  "file system\n");
         }

      return EXIT_FAILURE;
   }

   if (verbose) {
      printVerbose();
   }

   fclose(fileImage);
   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage, int partitionVal) {
   //TODO: CHECK THE BOOT IMAGE (See comment below)
   /* First check the boot block for magic number */

   /* Initialize the blocks */
   superBlock = malloc(sizeof(struct superblock));   
   
   /* If a partitition was specified, check the partition table for validity */
   if (partitionVal) {
      printf("HELI\n");
      partitionTable = malloc(sizeof(struct partitionEntry));
      
      /* Seek to partition sector and read the table */
      fseek(fileImage, PARTITION_TABLE_LOC, SEEK_SET);
      fread(partitionTable, sizeof(struct partitionEntry), 1, fileImage);

      /* Check if the partition table is valid before proceeding */
      if (partitionTable->bootind != BOOTABLE_MAGIC_NUM 
         || partitionTable->type != MINIX_PARTITION_TYPE) {
         fprintf(stderr, "Invalid partition table.\n");

         return INVALID_PARTITION;
      }
   }

   /* Seek to the next block (boot block is 2 sectors) */
   fseek(fileImage, SECTOR_SIZE * 2, SEEK_SET);
   fread(superBlock, sizeof(struct superblock), 1, fileImage);

   if (superBlock->magic != MAGIC_NUMBER) {
      printVerbose();
      fprintf(stderr, "Bad magic number. (0x%.4u)\n", superBlock->magic);
      fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
      
      return BAD_MAGIC_NUMBER;
   }

   printf("YAY\n");
   return EXIT_SUCCESS;
}

void printVerbose() {
   if (partitionTable != NULL) {
      printf("\n\nPartition table:\n");
      printf("       ----Start----      ------End------\n");
      printf("  Boot head  sec  cyl Type head  sec  cyl      First       Size\n");
      printf("  0x%x    %u    %u    %u 0x%x    %u   %u  %u         %u      %u",
         partitionTable->bootind, partitionTable->start_head, partitionTable->start_sec,
         partitionTable->start_cyl, partitionTable->type, partitionTable->end_head,
         partitionTable->end_sec, partitionTable->end_cyl, partitionTable->lFirst,
         partitionTable->size);
   }

   printf("\nSuperblock Contents:\n");
   printf("Stored Fields:\n");
   printf("  ninodes        %u\n", superBlock->ninodes);
   printf("  i_blocks       %u\n", superBlock->i_blocks);
   printf("  z_blocks       %u\n", superBlock->z_blocks);
   printf("  firstdata      %u\n", superBlock->firstdata);
   printf("  log_zone_size  %u (zone size: %u)\n",
    superBlock->log_zone_size, superBlock->blocksize << superBlock->log_zone_size);
   printf("  max_file       %u\n", superBlock->max_file);
   printf("  magic          %x\n", superBlock->magic);
   printf("  zones          %u\n", superBlock->zones);
   printf("  blocksize      %u\n", superBlock->blocksize);
   printf("  subversion     %u\n", superBlock->subversion);
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
