#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main(int argc, char *argv[]) {
   char *s;
   char printBuffer[100];
  /* 
   snprintf(printBuffer, 100, "Starting tryme\n");
   fputs(printBuffer, stdout);

   s = strdup("Tryme");
   puts(s);
   free(s);

   snprintf(printBuffer, 100, "Allocating 0\n");
   fputs(printBuffer, stdout);
   malloc(0);

   snprintf(printBuffer, 100, "Allocating 500\n");
   fputs(printBuffer, stdout);
   malloc(500);

   snprintf(printBuffer, 100, "Allocating 1000\n");
   fputs(printBuffer, stdout);
   malloc(1000);

   snprintf(printBuffer, 100, "Allocating 1500\n");
   fputs(printBuffer, stdout);
   malloc(1500);

   snprintf(printBuffer, 100, "Allocating 2000\n");
   fputs(printBuffer, stdout);
   malloc(2000);

   int i;
   void *ptr;
   for(i = 0; i < 100000; i++) {
      snprintf(printBuffer, 100, "Allocating %d\n", i);
      fputs(printBuffer, stdout);

      ptr = malloc(i);
      free(ptr);
   }
   */

   
   int i;
   int *ptr;


   for(i = 0; i < 8192; i++) {
      snprintf(printBuffer, 100, "Allocating %d\n", i);
      fputs(printBuffer, stdout);

      ptr = malloc(i);

      if(i>0)  {
       //  *ptr = 69;
         memset(ptr, 69, i);
      }


   //   free(ptr);
   }

   return 0;
}
