#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define DEFAULT_CHUNK_SIZE 65536
#define MIN_BLOCK_SIZE 16
#define TRUE 1
#define FALSE 0
#define PRINT_BUFFER_SIZE 100

typedef struct blockHeader {
   size_t size;
   int isFree;
   struct blockHeader *next;
} blockHeader;

void *head = NULL;
char printBuffer[PRINT_BUFFER_SIZE];

void printMemoryList(blockHeader *iterator) {
   int index = 0;

   while (iterator) {
      snprintf(printBuffer, 100, "%d: %lu size: %zu isFree: %d val: %d\n", 
            index++, (unsigned long)iterator, iterator->size, 
            iterator->isFree, *(int *)((void *)iterator + sizeof(blockHeader)));
      fputs(printBuffer, stdout);

      iterator = iterator->next;
   }
}

int modifySize(int size) {
   while (size % MIN_BLOCK_SIZE != 0) {
      size++;
   }

   return size;
}

void *findHeadLocation(void *heapLoc) {

   while ((unsigned long)heapLoc % MIN_BLOCK_SIZE != 0) {
      heapLoc++;
   }

   return heapLoc;
}

blockHeader *allocateMemory(size_t size, blockHeader *prev, 
      int isFirstAllocation) {

   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Allocating new memory\n");
      fputs(printBuffer, stdout);
   }

   blockHeader *newBlock;

   if (isFirstAllocation) {
      /* First Allocation. Make sure div by 16 then sbrk */

      void *heapLoc = sbrk(0);
      head = findHeadLocation(heapLoc);

      if (head != heapLoc) {
         /* Need to move head to divisible by 16 */
         
         void *wastedSpace = sbrk(head - heapLoc);
         
         if (wastedSpace == (void *) -1) {
            /* Error with sbrk */
            return NULL;
         }
      }
   }
   
   newBlock = sbrk(size + sizeof(blockHeader));

   if (newBlock == (void *) -1) {
      /* Error with sbrk */
      return NULL;
   }

   if (!isFirstAllocation) {
      /* Existing memory list */

      prev->next = newBlock;
   }

   newBlock->size = size;
   newBlock->next = NULL;
   newBlock->isFree = FALSE;

   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "allocated new memory at location %p with size %zu\n", newBlock, size);
      fputs(printBuffer, stdout);
   }

   return newBlock;
}

blockHeader *getFreeBlock(size_t size) {

   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Getting free Block\n");
      fputs(printBuffer, stdout);
   }

   blockHeader *iterator = head;
   blockHeader *prev = NULL;
/*
   snprintf(printBuffer, 100, "%p isFree: %d size: %zu\n", iterator, 
         iterator->isFree, iterator->size);
   fputs(printBuffer, stdout);
*/
   while (iterator != NULL && (!iterator->isFree || iterator->size < size)) {
      /* Looking for a free block big enough */

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough, allocate a new block */

      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, "Get free block did not find a big enough block\n");
         fputs(printBuffer, stdout);
      }

      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, prev, FALSE);
      }
      else {
         iterator = allocateMemory(DEFAULT_CHUNK_SIZE, prev, FALSE);
      }
   }

   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "getFreeBlock returning location %p with size %zu\n", iterator, size);
      fputs(printBuffer, stdout);
   }

   return iterator;
}

void *malloc(size_t size) {
   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my malloc\n");
      fputs(printBuffer, stdout);
   }

   if (size <= 0 ) {
      return NULL;
   }

   blockHeader *ret = NULL;
   blockHeader *newBlock;
   size_t leftoverSize, oldSize;

   size = modifySize(size);

   if (head == NULL) {
      /* First time calling malloc */ 
       
      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, "Head is null. First time calling malloc? \n");
         fputs(printBuffer, stdout);
      }

      if (size <= DEFAULT_CHUNK_SIZE) {
         ret = allocateMemory(DEFAULT_CHUNK_SIZE, NULL, TRUE);
      }
      else {
         ret = allocateMemory(size, NULL, TRUE);
      }

      if (ret == NULL) {
         /* Problem with sbrk */
         errno = ENOMEM;
         return NULL;
      }

      oldSize = ret->size;
      ret->size = size;
   } 
   
   else {   
      /* Existing memory List */ 

      ret = getFreeBlock(size); 

      if (ret == NULL) {
         /* Problem with sbrk */
         errno = ENOMEM;
         return NULL;
      }
      else {
         oldSize = ret->size;
         ret->isFree = 0;
         ret->size = size;
      }
   }

   leftoverSize = oldSize - size;
   if (leftoverSize > MIN_BLOCK_SIZE) {
      /* Break up block */
      newBlock = ret + size + sizeof(blockHeader);

      newBlock->next = ret->next;
      ret->next = newBlock;
      newBlock->size = leftoverSize - sizeof(blockHeader);
      newBlock->isFree = TRUE;

      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, "created new header at location %lu, size %zu\n", 
               (unsigned long)newBlock, newBlock->size);
         fputs(printBuffer, stdout);
      }
   }

   printMemoryList(head);

   return ret + sizeof(blockHeader);
}

void *calloc(size_t nmemb, size_t size) {
#ifdef DEBUG_MALLOC
   snprintf(printBuffer, 100, "Called my calloc\n");
   fputs(printBuffer, stdout);
#endif

   size_t newSize = nmemb * size;
   void *ret = malloc(newSize);

   if (ret == NULL) {
      /* Problem with sbrk */ 
      errno = ENOMEM;
      return NULL;
   }

   memset(ret, 0, newSize);
  
#ifdef DEBUG_MALLOC
   snprintf(printBuffer, 100, "MALLOC: calloc(%d,%d) => (ptr=%p, size=%d)\n", 
         nmemb, size, ret, newSize);
   fputs(printBuffer, stdout);
#endif

   return ret; 
}

blockHeader *findHeader(void *ptr) {

   blockHeader *iterator = head;

   if (ptr < head) {
      /* Invalid */
      return NULL;
   }

   while (iterator != NULL && ptr > ((void *)(iterator + iterator->size + 
               sizeof(blockHeader)))) {
      iterator = iterator->next;
   }
   
   return iterator;
}   

void free(void *ptr) {
   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my free\n");
      fputs(printBuffer, stdout);
   }

   if (ptr == NULL) {
      return;
   }

   blockHeader *blockToFree = findHeader(ptr);

   if (blockToFree != NULL) {
      
      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, "Freeing %p\n", blockToFree);
         fputs(printBuffer, stdout);
      }

      blockToFree->isFree = TRUE;


      if (blockToFree->next->isFree) {
         /* adjacent free blocks */
        
         //Might be better to go through the whole list and check for adjacent
         blockToFree->size = blockToFree->size + blockToFree->next->size;
         blockToFree->next = blockToFree->next->next;

      }
   }
}

void *realloc(void *ptr, size_t size) {
   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my realloc\n");
      fputs(printBuffer, stdout);
   }

   if (ptr == NULL) {
      return malloc(size);
   }
   if (size == 0) {
      free(ptr);
      return NULL; //I dont know if this is right
   }

   blockHeader *blockToRealloc = findHeader(ptr);
   void *newPtr; 

   if (blockToRealloc->size >= size) {
      /* Block is already big enough, dont do anything */ 
      return blockToRealloc;
   }

   /* Check for adjacent free block */
   else if (blockToRealloc->next != NULL && blockToRealloc->next->isFree) {
      blockToRealloc->size = blockToRealloc->size + blockToRealloc->next->size;
      blockToRealloc->next = blockToRealloc->next->next;

      //TODO: Probably should break this up

      return blockToRealloc + sizeof(blockHeader);
   }

   else {
      /* Allocate totally new block, mark old as free */ 

      newPtr = malloc(size);
      if (newPtr == NULL) {
         /* Problem with sbrk */ 
         errno = ENOMEM;
         return NULL;
      }

      //TODO: Make sure this is using the right size
      memcpy(newPtr, blockToRealloc + sizeof(blockHeader), 
            blockToRealloc->size);

      free(ptr);

      return newPtr;
   }
}
/*
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
            ptr = myMalloc(sizeof(int));
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
                  snprintf(printBuffer, 100, "Calling realloc on %p : 
                     val: %d\n", pointers[j], *(pointers[j]));
                  fputs(printBuffer, stdout);
                  ptr = myRealloc(pointers[j], rSize);
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
                  myFree(pointers[j]);

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
*/

