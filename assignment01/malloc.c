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
char printBuffer[PRINT_BUFFER_SIZE];
int debug = 0;

/*
 * Function to print the list of allocated memory 
 * to stderr
 */
void printMemoryList(blockHeader *iterator) {
   int index = 0;

   while (iterator) {
      snprintf(printBuffer, 100, 
            "%d: 0x%" PRIXPTR " size: %zu isFree: %d val: %d\n", 
            index++, (uintptr_t)iterator, iterator->size, 
            iterator->isFree, *(int *)((void *)iterator + sizeof(blockHeader)));
      fputs(printBuffer, stderr);

      iterator = iterator->next;
   }
}

/* 
 * Returns the next size_t divisible by MIN_BLOCK_SIZE (16)
 */
size_t modifySize(size_t size) {
   while (size % MIN_BLOCK_SIZE != 0) {
      size++;
   }

   return size;
}

/*
 * Finds the next address divisible by MIN_BLOCK_SIZE (16)
 */
void *findHeadLocation(void *heapLoc) {

   while ((uintptr_t)heapLoc % MIN_BLOCK_SIZE != 0) {
      heapLoc++;
   }

   return heapLoc;
}

/*
 * Function to call sbrk and allocate new memory. 
 * Returns the address of the new block.
 * Sets the size of the memory block and marks it as !free
 */
blockHeader *allocateMemory(size_t size, blockHeader *prev, 
      int isFirstAllocation) {

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
      fputs(printBuffer, stderr);
   }

   return newBlock;
}

/* 
 * Function to find the first free block large enough for 
 * parameter size. 
 * Returns a pointer to the header of the free block
 */
blockHeader *getFreeBlock(size_t size) {

 //  if (getenv("DEBUG_MALLOC")) {
   if(debug) {
      snprintf(printBuffer, 100, "Getting free Block of size %zu\n", size);
      fputs(printBuffer, stderr);
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
/*
      if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, 
               "Get free block did not find a big enough block\n");
         fputs(printBuffer, stderr);
      }
*/
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
      fputs(printBuffer, stderr);
   }

   return iterator;
}

void *malloc(size_t size) {
  // if (getenv("DEBUG_MALLOC")) {
  //    snprintf(printBuffer, 100, "Called my malloc\n");
  //    fputs(printBuffer, stderr);
  // }

   if (size <= 0 ) {
      return NULL;
   }

   blockHeader *ret = NULL;
   blockHeader *newBlock;
   size_t leftoverSize, oldSize;

   /* Make sure the size + header div by 16 */
   size = modifySize(size + sizeof(blockHeader));

   if (head == NULL) {
      /* First time calling malloc */ 
       
    /*  if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, 
               "Head is null. First time calling malloc? \n");
         fputs(printBuffer, stderr);
      }
*/
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
         fputs(printBuffer, stderr);

         errno = ENOMEM;
         return NULL;
      }
      
      oldSize = ret->size;
   } 
   
   else {   
      /* Existing memory List */ 

      ret = getFreeBlock(size); 

      if (ret == NULL) {
         /* Problem with sbrk */
      
         snprintf(printBuffer, 100, 
               "MALLOC - Problem with sbrk \n");
         fputs(printBuffer, stderr);

         errno = ENOMEM;
         return NULL;
      }
      else {
         oldSize = ret->size;
         ret->isFree = 0;
      }
   }

   leftoverSize = oldSize - size;
   
   if(debug) {
      snprintf(printBuffer, 100, "Leftover Size:%zu Old Size: %zu Size: %zu\n", 
            leftoverSize, oldSize, size);
      fputs(printBuffer, stderr);
   }

   if (leftoverSize > (MIN_BLOCK_SIZE + sizeof(blockHeader))) {
      /* Break up block */

      ret->size = size;

      newBlock = (void *)ret + size;

      newBlock->next = ret->next;
      ret->next = newBlock;
      newBlock->size = leftoverSize;
      newBlock->isFree = TRUE;
   }

 //  if (getenv("DEBUG_MALLOC")) {
   if(debug) {  
      printMemoryList(head);
   }

   void *pointerToBlock = (void *)ret + sizeof(blockHeader);
   assert((uintptr_t)pointerToBlock % 16 == 0);

   if(debug) {  
      snprintf(printBuffer, 100, 
            "Malloc Returning: 0x%" PRIXPTR " Header: 0x%" PRIXPTR "\n", 
            (uintptr_t)pointerToBlock, (uintptr_t)ret);
      fputs(printBuffer, stderr);
   }

   return pointerToBlock;
}

void *calloc(size_t nmemb, size_t size) {
/*
   if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my calloc\n");
      fputs(printBuffer, stderr);
   }
*/
   size_t newSize = nmemb * size;
   void *ret = malloc(newSize);

   if (ret == NULL) {
      /* Problem with sbrk */ 
      errno = ENOMEM;
      return NULL;
   }

   memset(ret, 0, newSize);
  
 /*  if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, 
            "MALLOC: calloc(%zu,%zu) => (ptr=0x%" PRIXPTR ", size=%zu)\n", 
            nmemb, size, (uintptr_t)ret, newSize);
      fputs(printBuffer, stderr);
   }*/

   return ret; 
}

/* 
 * Function to find the header of a block of memory given a pointer 
 * to memory for that header
 */
blockHeader *findHeader(void *ptr) {
   if (debug) {
      snprintf(printBuffer, 100, "Find header called with 0x%" PRIXPTR "\n", 
            (uintptr_t)ptr);
      fputs(printBuffer, stderr);
   }

   blockHeader *iterator = head;

   if (ptr < head) {
      /* Invalid */
      return NULL;
   }

   while (iterator != NULL && (uintptr_t)ptr >= 
         (uintptr_t)((void *)iterator + iterator->size + sizeof(blockHeader))) {
      /* If pointer is past the end of memory for this header, move on */

      iterator = iterator->next;
   }

   if (debug) {
      snprintf(printBuffer, 100, 
            "Find header returning 0x%" PRIXPTR "\n", (uintptr_t)iterator);
      fputs(printBuffer, stderr);
   }
   
   return iterator;
}   

void free(void *ptr) {
 /*  if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my free\n");
      fputs(printBuffer, stderr);
   }
*/
   if (ptr == NULL) {
      return;
   }

   blockHeader *blockToFree = findHeader(ptr);

   if (blockToFree != NULL) {
      
   /*   if (getenv("DEBUG_MALLOC")) {
         snprintf(printBuffer, 100, "Freeing 0x%" PRIXPTR "\n", 
               (uintptr_t)blockToFree);
         fputs(printBuffer, stderr);
      }
*/
      blockToFree->isFree = TRUE;


      if (blockToFree->next->isFree) {
         /* adjacent free blocks */
        
         blockToFree->size = blockToFree->size + blockToFree->next->size;
         blockToFree->next = blockToFree->next->next;

      }
   }
}

void *realloc(void *ptr, size_t size) {
  /* if (getenv("DEBUG_MALLOC")) {
      snprintf(printBuffer, 100, "Called my realloc\n");
      fputs(printBuffer, stderr);
   }
*/
   if (ptr == NULL) {
      return malloc(size);
   }
   if (size == 0) {
      free(ptr);
      return NULL;
   }

   blockHeader *blockToRealloc = findHeader(ptr);
   void *newPtr; 

   if (blockToRealloc->size >= (size + sizeof(blockHeader))) {
      /* Block is already big enough, dont do anything */

      if (debug) {
         snprintf(printBuffer, 100, "Block big enough, no moving\n");
         fputs(printBuffer, stderr);
      }

      return (void *)blockToRealloc + sizeof(blockHeader);
   }

   else if (blockToRealloc->next != NULL && blockToRealloc->next->isFree) {
      /* Check if there is enough space in adjacent blocks */

      size_t adjacentSize = 0; 
      int index = 0; 
      blockHeader *adjacentToTake[100];
      blockHeader *iterator = blockToRealloc->next;
      
      while (iterator != NULL && iterator->isFree) {
         adjacentSize += iterator->size;
         adjacentToTake[index++] = iterator;

         iterator = iterator->next;
      }

      if ((adjacentSize + blockToRealloc->size - sizeof(blockHeader)) >= size) {
   
         blockToRealloc->next = iterator;
         blockToRealloc->size = blockToRealloc->size + adjacentSize;

         return (void *)blockToRealloc + sizeof(blockHeader);
      }
   }
      
   /* Allocate totally new block, mark old as free */ 

   if (debug) {
      snprintf(printBuffer, 100, 
            "Mallocing new block. blockToRealloc: 0x%" PRIXPTR "\n", 
            (uintptr_t)blockToRealloc);
      fputs(printBuffer, stderr);
   }

   newPtr = malloc(size);
   if (newPtr == NULL) {
      /* Problem with sbrk */ 
      errno = ENOMEM;
      return NULL;
   }

   if (debug) {
      snprintf(printBuffer, 100, 
            "New Ptr: 0x%" PRIXPTR "\n", (uintptr_t)newPtr);
      fputs(printBuffer, stderr);
   }

   if (debug) {
      snprintf(printBuffer, 100, 
            "Copying: 0x%" PRIXPTR " To: 0x%" PRIXPTR "\n", 
            (uintptr_t)((void *)blockToRealloc + sizeof(blockHeader)), 
            (uintptr_t)newPtr);
      fputs(printBuffer, stderr);
   }

   memcpy(newPtr, (void *)blockToRealloc + sizeof(blockHeader), 
         blockToRealloc->size - sizeof(blockHeader));

   free(ptr);

   return newPtr;
}
