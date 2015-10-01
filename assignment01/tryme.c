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

  
   int i, j;
   int *ptr = malloc(1);
   *ptr = 69;
   char last[9000];
   int result;
/*
   for(i = 0; i < 8192; i++) {
      snprintf(printBuffer, 100, "Allocating %d\n", i);
      fputs(printBuffer, stdout);

      ptr = realloc(ptr, i);

      if(i>0)  {
         result = memcmp(ptr, last, i);
         if(result != 0) {
            snprintf(printBuffer, 100, "****ERROR **** Expected: %d Got: %d\n", 
               *last, *ptr);
            fputs(printBuffer, stdout);
           

     snprintf(printBuffer, 100, "Top:");
     fputs(printBuffer, stdout);
            
            j = 0;
            while(j < i) {
               snprintf(printBuffer, 100, "%d", (int)ptr[j]);
               fputs(printBuffer, stdout);

               j++;
            }

     snprintf(printBuffer, 100, "\n");
     fputs(printBuffer, stdout);
            

            return 0;
         }

         memset(ptr, 69, i);
      }

     snprintf(printBuffer, 100, "Bottom:");
     fputs(printBuffer, stdout);
            
      j = 0;
            while(j < i) {
               snprintf(printBuffer, 100, "%d", (int)ptr[j]);
               fputs(printBuffer, stdout);

               j++;
            }

     snprintf(printBuffer, 100, "\n");
     fputs(printBuffer, stdout);
            
      memcpy(last, ptr, i); 
   //   free(ptr);
   }*/

  

   for(i = 1; i < 8192; i++) {
      
      snprintf(printBuffer, 100, "Allocating %d\n", i);
      fputs(printBuffer, stdout);

      ptr = realloc(ptr, i);

      if(*ptr != 69) {
         snprintf(printBuffer, 100, "ERROR %d\n", *ptr);
         fputs(printBuffer, stdout);
         return 0;
      }

      //*ptr = 69;
   }

   return 0;
}
