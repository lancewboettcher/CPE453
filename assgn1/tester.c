#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

int main(int argc, char *argv[], char *envp[])
{
   int *ptr;
   int mychar;
   int *pointers[100];
   int index = 0;
   int i, j, rSize, rc;
   char c;
   
   while(1)
   {
          snprintf(printBuffer, 100, "Enter a character:\n");
          fputs(printBuffer, stdout);
          scanf(" %c %d", &c, &mychar);

          if(c == 'm') {
            ptr = malloc(sizeof(int));
            *ptr = mychar;
            pointers[index] = ptr;

            index++;
          }
          else if(c == 'r') {
            snprintf(printBuffer, 100, "Enter size and char to input\n");
            fputs(printBuffer, stdout);

            scanf(" %d %d", &rSize, &rc);

            for(j = 0; j <= index; j++) {
               if(*(pointers[j]) == mychar) {
                  snprintf(printBuffer, 100, 
                        "Calling realloc on %p : val: %d\n",
                        pointers[j], *(pointers[j]));
                  fputs(printBuffer, stdout);
                  ptr = realloc(pointers[j], rSize);
                 // *ptr = rc;
                  pointers[index++] = ptr;

                  break;
               }
            }

          }   
          else if(c == 'f') {

            for(j = 0; j <= index; j++) {
               if(*(pointers[j]) == mychar) {
                  snprintf(printBuffer, 100, "Calling free on %p : val: %d\n",
                        pointers[j], *(pointers[j]));
                  fputs(printBuffer, stdout);
                  free(pointers[j]);

                  break;
               }
            }
         
          }

          snprintf(printBuffer, 100, "\nPointers:\n");
          for(i = 0; i < index; i++) {
             snprintf(printBuffer, 100, "%d : %lu - %d \n", i, 
                   (unsigned long)pointers[i], *(pointers[i]));
             fputs(printBuffer, stdout);
          }

          snprintf(printBuffer, 100, "MALLOC RESPONSE ptr: %lu val: %d\n", 
                (unsigned long)ptr, *ptr);
          fputs(printBuffer, stdout);
   }

   return 0;
}
