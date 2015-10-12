#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"

static scheduler sched = NULL;
//scheduler sched = NULL;
static unsigned long uniqueId = 0;

/* Scheduler Variables */
static struct threadNode *threadHead; /* Pointer to the head of the scheduler list  */
static tid_t numThreads;       /* Total number of threads                    */
static thread curThread;       /* Current thread executing                   */
static rfile realContext;      /* rfile of the process when start is called  */ 

extern tid_t lwp_create(lwpfun function, void * args, size_t stackSize) {
   char *stackPtr;

   if (sched == NULL) {
     lwp_set_scheduler(NULL);
     sched->init();
   } 

   thread newThread = malloc(sizeof(context));
   newThread->stack = malloc(stackSize);
   newThread->stacksize = stackSize;
   newThread->tid = uniqueId++;

   /* Setup stack */
   stackPtr = (void *)(newThread->stack) + stackSize;
   
   //stackPtr -= sizeof(tid_t); /* only need to allocate size of one function */
   stackPtr -= sizeof(tid_t); /* return address                             */
   *((tid_t *)stackPtr) = (tid_t)lwp_exit;
   stackPtr -= sizeof(lwpfun); /* function to call                           */
   *((lwpfun *)stackPtr) = function;
 
   newThread->state.rbp = (tid_t)stackPtr;
   newThread->state.rsp = (tid_t)stackPtr;
   
   /* Setup registers */
   newThread->state.rdi = *((tid_t *)args);

   sched->admit(newThread);

   return newThread->tid;
}

extern void lwp_exit(void) {
   /* save one context */
   thread toFree = curThread;

   /* pick a thread to run */
   curThread = sched->next();

   /* if sched->next returns null there are no other threads, call lwp_stop() */
   if (!curThread) {
      lwp_stop();
   } else {
      /* load that thread's context */
      load_context(&(curThread->state));
   }
   
   /* free the old thread */
   free(toFree->stack);
   free(toFree);
}

extern tid_t lwp_gettid(void) {

   return curThread->tid;

}

extern void lwp_yield(void) {

   if (numThreads < 1) {
      return;
   }
   
   save_context(&(curThread->state));

   curThread = sched->next();

   load_context(&(curThread->state));
}

extern void lwp_start(void) {
   if (numThreads < 1) {
      return;
   }

   /* Save old Context */ 
   save_context(&realContext);

   curThread = sched->next();

   /* Load new context */ 
   load_context(&(curThread->state));
}

extern void lwp_stop(void) {

   if (numThreads > 0) {
      /* Only save the context if we have a thread*/ 
      save_context(&(curThread->state));
   }

   load_context(&realContext);
}

extern void lwp_set_scheduler(scheduler fun) {
   if (fun != NULL) {
      sched = fun;
   }
   else {
      sched->admit = &r_admit;
      sched->remove = &r_remove;
      sched->next = &r_next;
      sched->init = NULL;
      sched->shutdown = NULL;
   }
}

extern scheduler lwp_get_scheduler(void) {
   return sched;
}

extern thread tid2thread(tid_t tid) {

   struct threadNode *iterator = threadHead;
   while (iterator != NULL && iterator->thread.tid != tid) {
      iterator = iterator->next;
   }

   return &(iterator->thread);
}

/* Scheduler Functions */ 
void r_init() {
/*
   threadHead = NULL;
   numThreads = 0;
   currentThread = NULL;
*/
}

void r_shutdown() {
/*
   while (threadHead != NULL) {
      tmp = threadHead;
      threadHead = threadHead->next;
      
      free(tmp->thread->stack);
      free(tmp->thread);

      numThreads--;
   }
*/
}

void r_admit(thread new) {

   struct threadNode *iterator;

   if (threadHead == NULL) {
      /* First time calling admit. Empty list. */

      threadHead->thread = *new;
      threadHead->next = NULL; 
   }

   else {
      /* Existing thread list. Add to it */
      
      iterator = threadHead;

      struct threadNode *newNode = malloc(sizeof(struct threadNode));
      newNode->thread = *new;
      newNode->next = NULL;

      while (iterator->next) {
         /* Find the end of the list */
         iterator = iterator->next;
      }

      iterator->next = newNode;

   }

   numThreads++;
}

void r_remove(thread victim) {

   struct threadNode *iterator = threadHead;
   struct threadNode *prev = NULL;
   tid_t victimId = victim->tid;

   if (threadHead == NULL) {
      /* Empty list. No threads*/ 

      return;
   }

   while (iterator != NULL && iterator->thread.tid != victimId) {
      /* Find the victim */ 

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Didn't find the victim */

      return;
   }

   /* Found the victim. Remove it from the list */
   prev->next = iterator->next;

   /* Free the stack and thread 
   free(iterator->thread->stack);
   free(iterator->thread);
   */

   numThreads--;
}

thread r_next() {

   if (numThreads < 1) {
      return NULL;
   }

   else if (numThreads == 1) {
      return &(threadHead->thread);
   }

   else {
      /* There are > 1 threads in the list */ 

      struct threadNode *iterator = threadHead;

      /* find curThread's threadNode */ 
      while (iterator != NULL && iterator->thread.tid != curThread->tid) {
         iterator = iterator->next;
      }

      if (iterator->next != NULL) {
         return &(iterator->next->thread);
      }
      else {
         /* Circle back to the beginning of the list */ 

         return &(threadHead->thread);
      }
   }
}



