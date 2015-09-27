#include <stdio.h> 
#include <string.h>
#include <unistd.h>

#define DEFAULT_CHUNK_SIZE 65536
#define MIN_BLOCK_SIZE 20
#define TRUE 1
#define FALSE 0

typedef struct blockHeader {
   size_t size;
   int isFree;
   struct blockHeader *next;
} blockHeader;

blockHeader *head = NULL;

int main (int argc, char *argv[], char *envp[])
{

   return 0;
}

void *calloc(size t nmemb, size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my calloc\n");
#endif


   
}

void *malloc(size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my malloc\n");
#endif

   blockHeader *ret = NULL;
   blockHeader *newBlock;
   size_t leftoverSize;

   if (size <= 0 ) {
      return NULL;
   }

   if (head == NULL) {
      /* First time calling malloc */ 

      if (size <= DEFAULT_CHUNK_SIZE) {
         head = allocateMemory(DEFAULT_CHUNK_SIZE, NULL);
      }
      else {
         head = allocateMemory(size, NULL);
      }

      if (head == NULL) {
         /* Problem with sbrk */

         errno = ENOMEM;
         return NULL;
      }

      ret = head;
   } 
   
   else {   
      /* Existing memory List */ 

      ret = getFreeBlock(size); 

      if (head == NULL) {
         /* Problem with sbrk */

         errno = ENOMEM;
         return NULL;
      }
   }

   leftoverSize = ret->size - size - sizeof(blockHeader);
   if (leftoverSize > MIN_BLOCK_SIZE) {
      /* Break up block */
      newBlock = ret + size + sizeof(blockHeader);

      newBlock->next = ret->next;
      ret->next = newBlock;
      newBlock->size = leftoverSize - sizeof(blockHeader);
      newBlock->isFree = TRUE;
   }

   return ret;
}

void free(void *ptr) {
#ifdef DEBUG_MALLOC
   printf("Called my free\n");
#endif



}

void *realloc(void *ptr, size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my realloc\n");
#endif

   if (ptr == NULL) {
      return malloc(size);
   }




}

/* 
 * Helpers
*/

blockHeader *getFreeBlock(size_t size) {

   blockHeader *iterator = head;
   blockHeader *prev = NULL;

   while (iterator != NULL && !iterator->isFree && iterator->size < size) {
      /* Looking for a free block big enough */

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough, allocate a new block */
      
      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, prev);
      }
      else {
         iterator = allocateMemory(DEFAULT_CHUNK_SIZE);
      }
   }

   return iterator;
}

blockHeader *allocateMemory(size_t size, blockHeader *prev) {

   blockHeader *newBlock = sbrk(size + sizeof(blockHeader));

   if (newBlock == (void *) -1) {
      /* Error with sbrk */

      return NULL;
   }

   if (prev == NULL) {
      /* New memory list */

      head = newBlock;
   }
   else {
      /* Existing memory list */

      prev->next = newBlock;
   }

   newBlock->size = size;
   newBlock->next = NULL;
   newBlock->isFree = TRUE;

   return newBlock;
}
