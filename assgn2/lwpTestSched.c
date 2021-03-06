#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"

static scheduler sched;

/* LWP Variables */ 
static thread curThread;       /* Current thread executing                   */
static rfile realContext;      /* rfile of the process when start is called  */ 
static unsigned long uniqueId = 0;

/* Scheduler Variables */ 
static struct threadNode *threadHead;        /* Head of the scheduler list   */
static tid_t numThreads = 0;                 /* Total number of threads      */

extern tid_t lwp_create(lwpfun function, void * args, size_t stackSize) {
#ifdef DEBUG
   fprintf(stderr, "lwp_create called\n");
#endif
   char *stackPtr;

   /* TODO: We might not need this */
   if (sched == NULL) {
     lwp_set_scheduler(NULL);
     
     sched->init();
   }

   thread newThread = malloc(sizeof(context));
   newThread->stack = malloc(stackSize * sizeof(tid_t));
   newThread->stacksize = stackSize;
   newThread->tid = uniqueId++;

   /* Setup stack */
   stackPtr = (void *)(newThread->stack) + stackSize;
   
   stackPtr -= sizeof(tid_t); /* only need to allocate size of one function */
   stackPtr -= sizeof(tid_t); /* return address                             */
   *((tid_t *)stackPtr) = (tid_t)lwp_exit;
   stackPtr -= sizeof(lwpfun); /* function to call                           */
   *((lwpfun *)stackPtr) = function;

   stackPtr -= sizeof(lwpfun);
 
   newThread->state.rbp = (tid_t)stackPtr;
   newThread->state.rsp = (tid_t)stackPtr;
   
   /* Setup registers */
   newThread->state.rdi = (tid_t)args;

   sched->admit(newThread);

   return newThread->tid;
}

extern void lwp_exit(void) {
#ifdef DEBUG
   fprintf(stderr, "exit called\n");
#endif
   /* save one context */
   thread toFree = curThread;

   sched->remove(toFree);
   
#ifdef DEBUG
   fprintf(stderr, "exit attempting to call next\n");
#endif
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
#ifdef DEBUG
   fprintf(stderr, "yield called\n");
#endif

   /*
   if (numThreads < 1) {
      return;
   }
   */
   save_context(&(curThread->state));

   curThread = sched->next();

   //TODO: IDK
   if (curThread == NULL) {
      lwp_exit();
   }

   load_context(&(curThread->state));
}

extern void lwp_start(void) {
#ifdef DEBUG
   fprintf(stderr, "start called\n");
#endif
 
 /*  if (numThreads < 1) {
      return;
   }*/

   curThread = sched->next();

   if (curThread == NULL) {
      return;
   }

   /* Save old Context */ 
   save_context(&realContext); 

   /* Load new context */ 
   load_context(&(curThread->state));

#ifdef DEBUG
   if (curThread == NULL)
   fprintf(stderr, "curThread == NULL after start called\n");
#endif
}

extern void lwp_stop(void) {

 /*  if (numThreads > 0) {
      save_context(&(curThread->state));
   }*/

   save_context(&(curThread->state));

   load_context(&realContext);
}

extern void lwp_set_scheduler(scheduler fun) {
#ifdef DEBUG
   fprintf(stderr, "set sched called\n");
#endif
   if (fun != NULL) {
      sched = fun;
   }
   else 
   {
      sched = malloc(5 * sizeof(tid_t));
#ifdef DEBUG
      fprintf(stderr, "before admit\n");
#endif
      sched->admit = r_admit;
#ifdef DEBUG
      fprintf(stderr, "before remove\n");
#endif
      sched->remove = r_remove;
#ifdef DEBUG
      fprintf(stderr, "before next\n");
#endif
      sched->next = r_next;
      sched->init = r_init;
      sched->shutdown = r_shutdown;
   }
#ifdef DEBUG
   fprintf(stderr, "end set sched\n");
#endif
}

extern scheduler lwp_get_scheduler(void) {
   return sched;
}

extern thread tid2thread(tid_t tid) {

   struct threadNode *iterator = threadHead;
   while (iterator != NULL && (iterator->thread).tid != tid) {
      iterator = iterator->next;
   }

   return &(iterator->thread);
}

/* Scheduler Functions */ 
void r_init() {
}

void r_shutdown() {
}

void r_admit(thread new) {
#ifdef DEBUG
   fprintf(stderr, "admit called\n");
#endif
   struct threadNode *iterator;

   if (threadHead == NULL) {
      /* First time calling admit. Empty list. */
      threadHead = malloc(sizeof(struct threadNode));
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

#ifdef DEBUG
   fprintf(stderr, "r_remove called\n");
#endif
   struct threadNode *iterator = threadHead;
   struct threadNode *prev = NULL;
   tid_t victimId = victim->tid;

   if (threadHead == NULL) {
      /* Empty list. No threads*/ 

      return;
   }

   while (iterator != NULL && (iterator->thread).tid != victimId) {
      /* Find the victim */ 

      prev = iterator;
      iterator = iterator->next;
   }

   if (iterator == NULL) {
      /* Didn't find the victim */

      return;
   }

   /* Found the victim. Remove it from the list */
   if (iterator == threadHead) {
      /* threadHead was the victim */ 
      threadHead = threadHead->next;  
   }
   else {
      prev->next = iterator->next;
   }

   /* Free the stack and thread 
   free(iterator->thread->stack);
   free(iterator->thread);
   */

   numThreads--;
}

thread r_next() {
#ifdef DEBUG
   fprintf(stderr, "next is called\n");
#endif
   thread ret;

   if (numThreads < 1) {
      ret =  NULL;
   }
   
 //  else if (numThreads == 1) {
   else {
      ret = &(threadHead->thread);
   }
 /*  
   else {

      ret = &(threadHead->thread);
      
      
      struct threadNode *iterator = threadHead;

      while (iterator->next != NULL) {
         iterator = iterator->next;
      }
      iterator->next = threadHead;

      threadHead = threadHead->next;
      iterator->next->next = NULL;
   }
*/
   return ret;
}



