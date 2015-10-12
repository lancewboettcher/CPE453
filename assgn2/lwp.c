#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"

static struct scheduler sched = {NULL, NULL, r_admit, r_remove, r_next};
//scheduler sched = NULL;
unsigned long uniqueId = 0;

/* Scheduler Variables */
threadNode *threadHead; /* Pointer to the head of the scheduler list  */
tid_t numThreads;       /* Total number of threads                    */
thread curThread;       /* Current thread executing                   */
rfile realContext;      /* rfile of the process when start is called  */ 

extern tid_t lwp_create(lwpfun function, void * args, size_t stackSize) {
   void *stackPtr; 

   thread newThread = malloc(sizeof(context));
   newThread->stack = malloc(stackSize);
   newThread->stacksize = stackSize;
   newThread->tid = uniqueId++;

   /* Setup stack */
   stackPtr = (void *)(newThread->stack) + stackSize;
   
   stackPtr -= sizeof(tid_t); /* only need to allocate size of one function */
   *stackPtr = args;
   stackPtr -= sizeof(tid_t); /* return address                             */
   *stackPtr = (tid_t)lwp_exit;
   stackPtr -= sizeof(lwpfun); /* function to call                           */
   *stackPtr = function;
 
   newThread->stack.rbp = (tid_t)stackPtr;
   newThread->stack.rsp = (tid_t)stackPtr;
   
   /* Setup registers */
   newThread.state.rdi = *((tid_t *)args);

   sched.admit(newThread);

   return newThread->tid;
}

extern void lwp_exit(void) {
   /* save one context */
   thread toFree = curThread;

   /* pick a thread to run */
   curThread = sched->next;

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
      return NULL;
   }
   
   save_context(&(curThread->state));

   curThread = *(sched->next());

   load_context(&(curThread->state));
}

extern void lwp_start(void) {
   if (numThreads < 1) {
      return NULL;
   }

   /* Save old Context */ 
   save_context(&realContext);

   curThread = *(sched->next());

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
}

extern scheduler lwp_get_scheduler(void) {
   return sched;
}

extern thread tid2thread(tid_t tid) {

   threadNode *iterator = threadHead;
   while (iterator != NULL && iterator->thread.tid != tid) {
      iterator = iterator->next;
   }

   return iterator->thread;
}

/* Scheduler Functions */ 
void r_init() {

   threadHead = NULL;
   numThreads = 0;
   currentThread = NULL;

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

void r_remove(thread victim) {

   threadNode *iterator = threadHead;
   threadNode *prev = NULL;
   tid_t victimId = victim->tid;

   if (threadHead == NULL) {
      /* Empty list. No threads*/ 

      return NULL;
   }

   while (iterator != NULL && iterator->thread.tid != victimId) {
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
      return threadHead->thread;
   }

   else {
      /* There are > 1 threads in the list */ 

      threadNode *iterator = threadHead;

      /* find curThread's threadNode */ 
      while (iterator != NULL && iterator->thread.tid != curThread->tid) {
         iterator = iterator->next;
      }

      if (iterator->next != NULL) {
         return iterator->next->thread;
      }
      else {
         /* Circle back to the beginning of the list */ 

         return threadHead->thread;
      }
   }
}



