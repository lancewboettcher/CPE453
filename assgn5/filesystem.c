#ifdef FILESYSTEM_H
#define FILESYSTEM_H
#include "filesystem.h"
#endif

int strsplit(char *toSplit, char *delimiter, char **stringList) {
   //char *originalStr = strdup(toSplit);
   /*char **splitList;
   char *nextToken;
   int numStrings = 0; 

   while (nextToken = strsep(&toSplit != NULL) {
       
      numStrings++;
   }*/
   
   
   return 0;
}
/*
char **strsplit(const char* str, const char* delim, size_t* numtokens) {
   char *s = strdup(str);
   size_t tokens_alloc = 1;
   size_t tokens_used = 0;
   char **tokens = calloc(tokens_alloc, sizeof(char*));
   char *token, *rest = s;
   while ((token = strsep(&rest, delim)) != NULL) {
      if (tokens_used == tokens_alloc) {
         tokens_alloc *= 2;
         tokens = realloc(tokens, tokens_alloc * sizeof(char*));
      }
      tokens[tokens_used++] = strdup(token);
   }
   if (tokens_used == 0) {
      free(tokens);
      tokens = NULL;
   } else {
      tokens = realloc(tokens, tokens_used * sizeof(char*));
   }
   *numtokens = tokens_used;
   free(s);
   return tokens;
}*/
