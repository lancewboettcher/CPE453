#include <stdio.h>

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

