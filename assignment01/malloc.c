#include <stdio.h> 
#include <string.h>
#include <unistd.h>

#define DEFAULT_CHUNK_SIZE 65536
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

   if (size <= 0 ) {
      return NULL;
   }

   if (head == NULL) {
      /* First time calling malloc */ 
      head = allocateMemory(DEFAULT_CHUNK_SIZE, NULL);

      if (head == NULL) {
         return NULL;
      }
   }

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

blockHeader *getFreeBlock(size_t size, blockHeader **prev) {

   blockHeader *iterator = head;

   while (iterator != NULL && !iterator->isFree && iterator->size < size) {
      /* Looking for a free block big enough */

      *prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough */
      
      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, *prev);
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
      errno = ENOMEM;
      return NULL;
   }

   if (prev == NULL) {
      head = newBlock;
   }
   else {
      prev->next = newBlock;
   }

   newBlock->size = size + sizeof(blockHeader);
   newBlock->next = NULL;
   newBlock->isFree = FALSE;

   return newBlock;
}
