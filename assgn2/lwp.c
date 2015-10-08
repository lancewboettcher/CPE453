#include <stdio.h>

/* Scheduler Variables */
threadNode *threadHead;  /* Pointer to the head of the scheduler list */
tid_t numThreads;       /* Total number of threads                    */
threadNode curThread;   /* Current thread executing                   */

extern tid_t lwp_create(lwpfun,void *,size_t) {

}

extern void  lwp_exit(void) {

}

extern tid_t lwp_gettid(void) {

}

extern void  lwp_yield(void) {

}

extern void  lwp_start(void) {

}

extern void  lwp_stop(void) {

}

extern void  lwp_set_scheduler(scheduler fun) {

}

extern scheduler lwp_get_scheduler(void) {

}

extern thread tid2thread(tid_t tid) {

}

/* Scheduler Functions */ 
void init() {

   threadHead = NULL;
   numThreads = 0;
   currentThread = NULL;

}

void shutdown() {

   threadNode *prev = NULL;

   while (threadHead != NULL) {
      prev = threadHead 
      remove(prev);

      threadHead = threadHead->next;
   }


}

void admit(thread new) {

   threadNode *iterator;

   if (threadHead == NULL) {
      /* First time calling admit. Empty list. */

      threadHead = new;
      threadHead->next = NULL; 
   }

   else {
      /* Existing thread list. Add to it */
      
      iterator = threadHead;

      while (iterator->next) {
         /* Find the end of the list */
         iterator = iterator->next;
      }

      iterator->next = new;

   }

   numThreads++;
}

void remove(thread victim) {

   threadNode *iterator = threadHead;
   threadNode *prev = NULL;
   tid_t victimId = victim->tid;

   if (threadHead == NULL) {
      /* Empty list. No threads*/ 

      return NULL;
   }

   while (iterator != NULL && iterator->tid != victimId) {
      /* Find the victim */ 

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Didn't find the victim */

      return NULL;
   }

   /* Found the victim. Remove it from the list */
   prev->next = iterator->next;

   /* Free the stack and thread*/ 
   free(iterator->thread->stack);
   free(iterator->thread);
   
   numThreads--;
}

thread next() {


}

