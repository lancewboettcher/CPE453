#include <stdio.h> 
#include <string.h>
#include <unistd.h>

#define DEFAULT_CHUNK_SIZE 65536

typedef struct blockHeader {
   size_t size;
   int isFree;
   struct blockHeader *next;
} blockHeader;

void *head = NULL;

int main (int argc, char *argv[], char *envp[])
{

   return 0;
}

void *calloc(size t nmemb, size t size) {
#ifdef DEBUG_MALLOC
   printf("Called my calloc\n");
#endif
   
}

void *malloc(size t size) {
#ifdef DEBUG_MALLOC
   printf("Called my malloc\n");
#endif



}

void free(void *ptr) {
#ifdef DEBUG_MALLOC
   printf("Called my free\n");
#endif


}

void *realloc(void *ptr, size t size) {
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

struct blockHeader getFreeBlock(size_t size, ) {



}

struct blockHeader allocateMoreMemory(size_t size, ) {


}
