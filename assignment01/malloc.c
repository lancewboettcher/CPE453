#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>

#define DEFAULT_CHUNK_SIZE 65536
#define MIN_BLOCK_SIZE 16
#define TRUE 1
#define FALSE 0
#define PRINT_BUFFER_SIZE 100

typedef struct blockHeader {
   size_t size;
   int isFree;
   struct blockHeader *next;
   int dummyToMake16;
} blockHeader;

void *head = NULL;
void *topOfHeap = NULL;
char printBuffer[PRINT_BUFFER_SIZE];
int debug = 0;

void printMemoryList(blockHeader *iterator) {
   int index = 0;

   while (iterator) {
      snprintf(printBuffer, 100, 
            "%d: 0x%" PRIXPTR " size: %zu isFree: %d val: %d\n", 
            index++, (uintptr_t)iterator, iterator->size, 
            iterator->isFree, *(int *)((void *)iterator + sizeof(blockHeader)));
      fputs(printBuffer, stdout);

      iterator = iterator->next;
   }
}

size_t modifySize(size_t size) {
   while (size % MIN_BLOCK_SIZE != 0) {
      size++;
   }

   return size;
}

void *findHeadLocation(void *heapLoc) {

   while ((uintptr_t)heapLoc % MIN_BLOCK_SIZE != 0) {
      heapLoc++;
   }

   return heapLoc;
}

blockHeader *allocateMemory(size_t size, blockHeader *prev, 
      int isFirstAllocation) {

   //if (getenv("DEBUG_MALLOC")) {
   if(debug) {
      snprintf(printBuffer, 100, "*****Allocating new memory******\n");
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
   
   newBlock = sbrk(size);
   
   topOfHeap=sbrk(0);

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
      snprintf(printBuffer, 100, 
            "allocated new memory at location 0x%" PRIXPTR "with size %zu\n",
            (uintptr_t)newBlock, newBlock->size);
      fputs(printBuffer, stdout);
   }

   return newBlock;
}

blockHeader *getFreeBlock(size_t size) {

 //  if (getenv("DEBUG_MALLOC")) {
   if(debug) {
      snprintf(printBuffer, 100, "Getting free Block of size %zu\n", size);
      fputs(printBuffer, stdout);
   }

   blockHeader *iterator = head;
   blockHeader *prev = NULL;
   
   while (iterator != NULL && (!iterator->isFree || iterator->size < size)) {
      /* Looking for a free block big enough */

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough, allocate a new block */

      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, 
               "Get free block did not find a big enough block\n");
         fputs(printBuffer, stdout);
      }

      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, prev, FALSE);
      }
      else {
         iterator = allocateMemory(DEFAULT_CHUNK_SIZE, prev, FALSE);
      }
   }

 //  if (getenv("DEBUG_MALLOC")) {
   if(debug) {
      snprintf(printBuffer, 100, 
            "getFreeBlock returning location 0x%" PRIXPTR " with size %zu\n", 
            (uintptr_t)iterator, iterator->size);
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

   size = modifySize(size + sizeof(blockHeader));

   if (head == NULL) {
      /* First time calling malloc */ 
       
      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, 
               "Head is null. First time calling malloc? \n");
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

         snprintf(printBuffer, 100, 
               "MALLOC - Problem with sbrk \n");
         fputs(printBuffer, stdout);

         errno = ENOMEM;
         return NULL;
      }
      
      oldSize = ret->size;
     // ret->size = size;
   } 
   
   else {   
      /* Existing memory List */ 

      ret = getFreeBlock(size); 

      if (ret == NULL) {
         /* Problem with sbrk */
      
         snprintf(printBuffer, 100, 
               "MALLOC - Problem with sbrk \n");
         fputs(printBuffer, stdout);

         errno = ENOMEM;
         return NULL;
      }
      else {
         oldSize = ret->size;
         ret->isFree = 0;
      
      //   ret->size = size;
      }
   }

   leftoverSize = oldSize - size;
   
   if(debug) {
      snprintf(printBuffer, 100, "Leftover Size:%zu Old Size: %zu Size: %zu\n", 
            leftoverSize, oldSize, size);
      fputs(printBuffer, stdout);
   }

   if (leftoverSize > (MIN_BLOCK_SIZE + sizeof(blockHeader))) {
      /* Break up block */

      ret->size = size;

      //TODO: Might be able to check if real size is smaller
      newBlock = (void *)ret + size;

      newBlock->next = ret->next;
      ret->next = newBlock;
      newBlock->size = leftoverSize;
      newBlock->isFree = TRUE;

  //    if (getenv("DEBUG_MALLOC")) {
 /*     if(debug) {
         snprintf(printBuffer, 100, 
               "***Top of heap is 0x%" PRIXPTR " created new 
               header at location 0x%" PRIXPTR ", size %zu\n",
               (uintptr_t)topOfHeap, (uintptr_t)newBlock, newBlock->size);
         fputs(printBuffer, stdout);
      }*/
   }

 //  if (getenv("DEBUG_MALLOC")) {
   if(debug) {  
      printMemoryList(head);
   }

   void *pointerToBlock = (void *)ret + sizeof(blockHeader);
   assert((uintptr_t)pointerToBlock % 16 == 0);
   return pointerToBlock;
}

void *calloc(size_t nmemb, size_t size) {


   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my calloc\n");
      fputs(printBuffer, stdout);
   }

   size_t newSize = nmemb * size;
   void *ret = malloc(newSize);

   if (ret == NULL) {
      /* Problem with sbrk */ 
      errno = ENOMEM;
      return NULL;
   }

   memset(ret, 0, newSize);
  
   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, 
            "MALLOC: calloc(%zu,%zu) => (ptr=0x%" PRIXPTR ", size=%zu)\n", 
            nmemb, size, (uintptr_t)ret, newSize);
      fputs(printBuffer, stdout);
   }

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
         snprintf(printBuffer, 100, "Freeing 0x%" PRIXPTR "\n", 
               (uintptr_t)blockToFree);
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

