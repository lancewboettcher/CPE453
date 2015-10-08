#include <stdio.h>
#include "lwp.h"

scheduler sched = NULL;
unsigned long uniqueId = 0;

extern tid_t lwp_create(lwpfun function, void * args, size_t stackSize) {
   void *stackPtr; 

   if (!sched) {
      sched = malloc(sizeof(scheduler));
   }

   thread *newThread = malloc(sizeof(context));
   newThread->stack = malloc(stackSize);
   newThread->stacksize = stackSize;
   newThread->tid = uniqueId++;

   /* Setup stack */
   stackPtr = (void *)newThread->stack + stackSize;
   
   stackPtr -= 7 * sizeof(tid_t); /* arguments */
   *stackPtr = args;
   stackPtr -= sizeof(tid_t); /* return address */
   *stackPtr = (tid_t)lwp_exit;
   stackPtr -= sizeof(lwpfun); /* function to call */
   *stackPtr = function;
 
   newThread->stack.rbp = (tid_t)stackPtr;
   newThread->stack.rsp = (tid_t)stackPtr;
   
   /* Setup registers? */

   sched.admit(newThread);
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

