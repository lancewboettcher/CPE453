#include "filesystem.h"

#define TRUE 1
#define FALSE 0

/* Global Variables */
superblock *superBlock = NULL;

/* Function Prototypes */
void printHelp(void);
int initFileSystem(FILE *fileImage);

int main (int argc, char *argv[]) {
   int i = 1;
   int verbose = FALSE;
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

   if (fileImage == NULL) {
      fprintf(stderr, "Unable to open file specified\n");

      return EXIT_FAILURE;
   }
   
   if (initFileSystem(fileImage)) {
      fprintf(stderr, "An error occured when attempting to initialize the "
            "file system\n");

      return EXIT_FAILURE;
   }

   if (verbose) {
      printf("Hey we got here\n");
   }

   return EXIT_SUCCESS;
}

int initFileSystem(FILE *fileImage) {
   /* First check the boot block for magic number */

   /* Check if the partition table is valid before proceeding */

   superBlock = malloc(sizeof(superblock));


   return EXIT_SUCCESS;
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
