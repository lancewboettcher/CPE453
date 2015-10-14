#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include <inttypes.h>
static scheduler sched;

/* LWP Variables */
static thread curThread;       /* Current thread executing                   */
static rfile realContext;      /* rfile of the process when start is called  */
static unsigned long uniqueId = 0;

/* Scheduler Variables */
static struct threadNode *threadHead;        /* Head of the scheduler list   */
static tid_t numThreads = 0;                 /* Total number of threads      */
static struct threadNode *endOfList;         /* Tail of the scheduler list   */

extern tid_t lwp_create(lwpfun function, void * args, size_t stackSize) {
   char *stackPtr;

   if (sched == NULL) {
     lwp_set_scheduler(NULL);
   }
   
   /* malloc size of thread context */
   thread newThread = (thread)malloc(sizeof(context));

   /* malloc size of stack */
   newThread->stack = malloc(stackSize * sizeof(tid_t));
   
   /* Setup stack */
   stackPtr = (char *)(newThread->stack) + sizeof(tid_t) * stackSize;
   
   /* return address */
   stackPtr -= sizeof(tid_t);
   *((tid_t *)stackPtr) = (tid_t)lwp_exit;
   
   /* function to call */
   stackPtr -= sizeof(lwpfun);
   *((lwpfun *)stackPtr) = function;

   /* set up the stack pointer below the function so the function is the
    * first thing popped off the stack */
   stackPtr -= sizeof(tid_t);
 
   /* set newThread fields */
   newThread->stacksize = stackSize;
   newThread->tid = uniqueId++;
   newThread->state.rbp = (tid_t)stackPtr;
   newThread->state.rsp = (tid_t)stackPtr;
   
   /* Setup registers */
   newThread->state.rdi = (tid_t)args;
   sched->admit(newThread);

   return newThread->tid;
}

extern void lwp_exit(void) {
   /* save one context */
   thread toFree = curThread;
 
   /* remover thread from scheduler */
   if (toFree != NULL) {
     sched->remove(toFree);
   }
   
   /* pick a thread to run */
   curThread = sched->next();

   /* if sched->next returns null there are no other threads, call lwp_stop() */
   if (!curThread) {
      lwp_stop();
   } else {
      /* load that thread's context */
      load_context(&(curThread->state));
   }

   /* free resources */
   if (toFree != NULL) {
      free(toFree->stack);
      free(toFree);
   }
}

extern tid_t lwp_gettid(void) {
   return curThread->tid;
}

extern void lwp_yield(void) {
   /* save current context */
   save_context(&(curThread->state));
   
   /* grab the next thread in the scheduler list */
   curThread = sched->next();
   
   /* if no threads left in scheduler, call lwp_exit() */
   if (curThread == NULL) {
      lwp_exit();
   }

   /* load the new context */
   load_context(&(curThread->state));
}

extern void lwp_start(void) {
   /* make sure the scheduler is set even if no threads were created */
   if (sched == NULL) {
     lwp_set_scheduler(NULL);
   }

   /* Save old Context */ 
   save_context(&realContext);

   /* grab the next thread in the scheduler list */
   curThread = sched->next();
   
   /* Load new context */ 
   if (curThread != NULL) {
      load_context(&(curThread->state));
   }
}

extern void lwp_stop(void) {
   /* save the current thread's context */
   if (curThread != NULL) {
      save_context(&(curThread->state));
   }

   /* shutdown the scheduler if the scheduler has a shutdown function */
   if (sched->shutdown != NULL) {
      sched->shutdown();
   }
   
   /* restore the context to what it was before threads were called */
   load_context(&realContext);
}

extern void lwp_set_scheduler(scheduler fun) {
   //TODO: clean up  
   if (sched != NULL && fun != NULL) {
      /* if scheduler is previously initialized, swictch scheduler */
      thread tempThread;

      /* grab all of the threads from the first scheduler and admit 
       * it to the new one */
      while ((tempThread = sched->next())) {
         fun->admit(tempThread);
         sched->remove(tempThread);
      }

      /* if the the scheduler has a shutdown function, shut it down */
      if (sched->shutdown) {
         sched->shutdown();
      }
      
      /* set the global sched variable to the new scheduler */
      sched = fun;
   }
   else {
      /* if a scheduler function is provided, set the sched 
       * variable to it */
      if (fun != NULL) {
         sched = fun;
      }
      else 
      {
         /* no scheduler provided, malloc and use the scheduler
          * we created */
         sched = malloc(5 * sizeof(tid_t));
         sched->admit = r_admit;
         sched->remove = r_remove;
         sched->next = r_next;
         sched->init = r_init;
         sched->shutdown = r_shutdown;
      }

   }
  
   /* if the the scheduler has an init function, initialize it */
   if (sched->init != NULL) {
      sched->init();
   }
   
}

extern scheduler lwp_get_scheduler(void) {
   return sched;
}

extern thread tid2thread(tid_t tid) {
   /* iterate through the scheduler list and return the thread that 
    * matched the tid */
   struct threadNode *iterator = threadHead;
   while (iterator != NULL && (iterator->thread).tid != tid) {
      iterator = iterator->next;
   }

   return &(iterator->thread);
}

/* Scheduler Functions */ 

void r_init() {
   /* nothing to initialize */
}

void r_shutdown() {
   /* nothing to shut down */
}

void r_admit(thread new) {
   struct threadNode *iterator;

   if (threadHead == NULL) {
      /* First time calling admit (list is empty). Initialize list nodes 
       * and set values to thread */
      threadHead = malloc(sizeof(struct threadNode));
      threadHead->thread = *new;
      threadHead->next = NULL; 

      /* only one node in the list, end of list points to the threadHead */
      endOfList = threadHead;
   }

   else {
      /* Existing thread list. Add to it */
      iterator = threadHead;
      
      /* create a node for the thread to add to the list */
      struct threadNode *newNode = malloc(sizeof(struct threadNode));
      newNode->thread = *new;
      newNode->next = NULL;
      
      /* set the endOfList point to the end of the list for efficiency */
      endOfList->next = newNode;
      endOfList = newNode;
   }

   numThreads++;
}

void r_remove(thread victim) {
   struct threadNode *iterator = threadHead;
   struct threadNode *prev = NULL;
   tid_t victimId = victim->tid;

   if (threadHead == NULL || victim == NULL) {
      /* Empty list or NULL victim */ 
      return;
   }

   /* traverse the scheduler list to find the victim */
   while (iterator != NULL && (iterator->thread).tid != victimId) {
      /* keep track of the previous node visited for linking purposes */
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
      /* removed last node of the list, set end of list to prev node */
      if (iterator->next == NULL) {
         endOfList = prev;
      }
      prev->next = iterator->next;
   }
   
   numThreads--;
}

thread r_next() {
   thread ret;

   /* if no threads left in scheduler, return NULL */
   if (numThreads < 1) {
      ret =  NULL;
   }
   /* if one thread left in list, return the threadHead */
   else if (numThreads == 1) {
      ret = &(threadHead->thread);
   }
   else {
      /* More than 1 thread in the list */ 

      ret = &(threadHead->thread);
      
      /* Move threadHead to the back of the list */ 
      endOfList->next = threadHead;
      endOfList = threadHead;

      threadHead = threadHead->next;

      endOfList->next = NULL;
   }

   return ret;
}



