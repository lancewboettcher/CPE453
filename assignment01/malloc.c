#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define DEFAULT_CHUNK_SIZE 65536
#define MIN_BLOCK_SIZE 16
#define TRUE 1
#define FALSE 0

typedef struct blockHeader {
   size_t size;
   int isFree;
   struct blockHeader *next;
} blockHeader;

void *head = NULL;

void printMemoryList(blockHeader *iterator) {
   //blockHeader *iterator = head;
   int index = 0;

   while(iterator) {
      printf("%d: %p size: %zu isFree: %d val: %d\n", index++, iterator, iterator->size, 
            iterator->isFree, *(int *)(iterator));

      iterator = iterator->next;
   }
}

blockHeader *allocateMemory(size_t size, blockHeader *prev) {

   printf("Allocating new memory\n");

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
   newBlock->isFree = FALSE;

   printf("allocated new memory at location %p with size %zu\n", newBlock, size);

   return newBlock;
}

blockHeader *getFreeBlock(size_t size) {

   printf("Getting free Block\n");

   blockHeader *iterator = head;
   blockHeader *prev = NULL;

   //printMemoryList(iterator);
   printf("%p isFree: %d size: %zu\n", iterator, iterator->isFree, iterator->size);

   while (iterator != NULL && (!iterator->isFree || iterator->size < size)) {
      /* Looking for a free block big enough */

      printf("in while\n");

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough, allocate a new block */

      printf("Get free block did not find a big enough block\n");
      
      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, prev);
      }
      else {
         iterator = allocateMemory(DEFAULT_CHUNK_SIZE, prev);
      }
   }

   printf("getFreeBlock returning location %p with size %zu\n", iterator, size);

   return iterator;
}

int modifySize(int size) {
   while(size % MIN_BLOCK_SIZE != 0) {
      size++;
   }

   return size;
}

void *myMalloc(size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my malloc\n");
#endif

   blockHeader *ret = NULL;
   blockHeader *newBlock;
   size_t leftoverSize, oldSize;

   if (size <= 0 ) {
      return NULL;
   }

   size = modifySize(size);

   if (head == NULL) {
      /* First time calling malloc */ 

      printf("Head is null. First time calling malloc? \n");

      if (size <= DEFAULT_CHUNK_SIZE) {
         ret = allocateMemory(DEFAULT_CHUNK_SIZE, NULL);
      }
      else {
         ret = allocateMemory(size, NULL);
      }

      if (ret == NULL) {
         /* Problem with sbrk */

         errno = ENOMEM;
         return NULL;
      }

      head = ret;
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

   leftoverSize = oldSize - size - sizeof(blockHeader);
   if (leftoverSize > (MIN_BLOCK_SIZE + sizeof(blockHeader))) {
      /* Break up block */
      newBlock = ret + size + sizeof(blockHeader);

      newBlock->next = ret->next;
      ret->next = newBlock;
      newBlock->size = leftoverSize - sizeof(blockHeader);
      newBlock->isFree = TRUE;

      printf("created new header at location %p, size %zu\n", newBlock, newBlock->size);
   }

   printMemoryList(head);

   return ret + sizeof(blockHeader);
}

void *calloc(size_t nmemb, size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my calloc\n");
#endif

   size_t newSize = nmemb * size;
   void *ret = myMalloc(newSize);
   memset(ret, 0, newSize);
  
#ifdef DEBUG_MALLOC
   printf("MALLOC: calloc(%d,%d) => (ptr=%p, size=%d)\n", nmemb, size, ret, newSize);
#endif

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
      return myMalloc(size);
   }

}

int main(int argc, char *argv[], char *envp[])
{
   int *ptr;
   int mychar;
   int *pointers[100];
   int index = 0;
   int i;
   
   ptr = myMalloc(sizeof(int));
   *ptr = 69;

   printf("return ptr: %p val: %d\n", ptr, *ptr);

   while(1)
   {
          printf("Enter a character:\n");
          scanf("%d", &mychar);

          ptr = myMalloc(sizeof(int));
          *ptr = mychar;
          pointers[index] = ptr;

          for(i = 0; i <= index; i++) {
             printf("%d : %d \n", i, *(pointers[i]));
          }

          index++;

          printf("MALLOC RESPONSE ptr: %p val: %d\n", ptr, *ptr);

   }

   return 0;
}


