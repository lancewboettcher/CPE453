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

   while (iterator) {
      printf("%d: %p size: %zu isFree: %d val: %d\n", index++, iterator, iterator->size, 
            iterator->isFree, *(int *)(iterator));

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

blockHeader *allocateMemory(size_t size, blockHeader *prev, int isFirstAllocation) {

   printf("Allocating new memory\n");

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

   printf("allocated new memory at location %p with size %zu\n", newBlock, size);

   return newBlock;
}

blockHeader *getFreeBlock(size_t size) {

   printf("Getting free Block\n");

   blockHeader *iterator = head;
   blockHeader *prev = NULL;

   printf("%p isFree: %d size: %zu\n", iterator, iterator->isFree, iterator->size);

   while (iterator != NULL && (!iterator->isFree || iterator->size < size)) {
      /* Looking for a free block big enough */

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Nothing big enough, allocate a new block */

      printf("Get free block did not find a big enough block\n");
      
      if (size > DEFAULT_CHUNK_SIZE) {
         iterator = allocateMemory(size, prev, FALSE);
      }
      else {
         iterator = allocateMemory(DEFAULT_CHUNK_SIZE, prev, FALSE);
      }
   }

   printf("getFreeBlock returning location %p with size %zu\n", iterator, size);

   return iterator;
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

blockHeader *findHeader(void *ptr) {

   blockHeader *iterator = head;

   if (ptr < head) {
      /* Invalid */
      return NULL;
   }

   while (iterator != NULL && ptr > ((void *)(iterator + iterator->size + sizeof(blockHeader)))) {
      iterator = iterator->next;
   }
   
   return iterator;
}   

void myFree(void *ptr) {
#ifdef DEBUG_MALLOC
   printf("Called my free\n");
#endif

   if (ptr == NULL) {
      return;
   }

   blockHeader *blockToFree = findHeader(ptr);

   if (blockToFree != NULL) {
      printf("Freeing %p\n", blockToFree);
      blockToFree->isFree = TRUE;


      if (blockToFree->next->isFree) {
         /* adjacent free blocks */
         
         blockToFree->next = blockToFree->next->next;
      }
   }
}

void *realloc(void *ptr, size_t size) {
#ifdef DEBUG_MALLOC
   printf("Called my realloc\n");
#endif

   if (ptr == NULL) {
      return myMalloc(size);
   }

   return NULL;
}

int main(int argc, char *argv[], char *envp[])
{
   int *ptr;
   int mychar;
   int *pointers[100];
   int index = 0;
   int i, j;
   char c;
   
   while(1)
   {
          printf("Enter a character:\n");
          scanf(" %c %d", &c, &mychar);

          if(c != 'f') {
            ptr = myMalloc(sizeof(int));
            *ptr = mychar;
            pointers[index] = ptr;

            index++;
          }
          else {

            for(j = 0; j <= index; j++) {
               if(*(pointers[j]) == mychar) {
                  printf("Calling free on %p : val: %d\n", pointers[j], *(pointers[j]));
                  myFree(pointers[j]);

                  break;
               }
            }
         
          }

          printf("\nPointers:\n");
          for(i = 0; i < index; i++) {
             printf("%d : %p - %d \n", i, pointers[i], *(pointers[i]));
          }

          printf("MALLOC RESPONSE ptr: %lu val: %d\n", (unsigned long)ptr, *ptr);

   }

   return 0;
}


