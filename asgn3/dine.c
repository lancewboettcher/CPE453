/* 
 * CPE 453 - Assignment 3
 * Dining Philosophers 
 *
 * Lance Boettcher (lboettch)
 *
*/ 

#include <errno.h> 
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h>
#include <time.h>
#include <limits.h>

#define NUM_PHILOSOPHERS 5 
#define NUM_FORKS 5
#define DEFAULT_CYCLES 1   /* Default number of eat-think cycles */ 
#define LEFT_FORK (id+1)%NUM_PHILOSOPHERS
#define RIGHT_FORK (id)
#define FORK_DOWN 100      /* Random int value to indicate fork is down */
#define EATING 0
#define THINKING 1
#define CHANGING 2

void *philosopher(void *id);
void put_forks(int id);
void take_forks(int id);
void takeLeftFork(int id);
void takeRightFork(int id);
void eat(int id);
void think(int id);
void test(int id);
void printHeader();
void printFooter();
void printStatusLine();
void dawdle();
void initLocks();
void destroyLocks();

int numCycles;                    /* Number of eat-think cycles to go through */
int state[NUM_PHILOSOPHERS];      /* EATING THINKING or CHANGING */ 
int printForks[NUM_FORKS];        /* index = fork #. value = phil # holding it*/
pthread_mutex_t forks[NUM_FORKS]; /* Locks on the forks */ 
pthread_mutex_t printLock;        /* Printing function lock */ 
pthread_mutex_t takeForkLock;     /* take_forks lock */ 
pthread_mutex_t putForkLock;      /* put_forks lock */ 

void *philosopher(void *id) {
   /* We know that this is always going to be a int */
   int myId = *(int *)id;
   int i; 

   /* Cycle through eating and thinking */ 
   for (i = 0; i < numCycles; i++) {
      take_forks(myId);
      eat(myId);
      put_forks(myId);
      think(myId);

      /* If we don't set the state to changing here they will be 
       * thinking while waiting for forks even though theyre hungry */
      state[myId] = CHANGING;
   }

   return NULL;
}

void take_forks(int id) {
   /* Enter critical section */ 
   if (pthread_mutex_lock(&takeForkLock) != 0) {
      fprintf(stderr, "P thread error entering take_forks \n");
      exit(-1);
   }

   /* Philosopher is hungry */  
   state[id] = CHANGING;

   printStatusLine();
   
   if (id % 2 == 0) {
      /* Even philosopher, take right fork first */ 
      takeRightFork(id);
      takeLeftFork(id);
   }
   else {
      /* Odd philosopher, take left fork first */ 
      takeLeftFork(id);
      takeRightFork(id);
   }      

   /* Leave Critical section */ 
   if (pthread_mutex_unlock(&takeForkLock) != 0) {
      fprintf(stderr, "P thread error leaving take_fork \n");
      exit(-1);
   }
}

void put_forks(int id) {
   /* Enter Critical Region */ 
   if (pthread_mutex_lock(&putForkLock) != 0) {
      fprintf(stderr, "P thread error entering put_forks \n");
      exit(-1);
   }

   /* Philosopher wants to think */ 
   state[id] = CHANGING;

   printStatusLine();

   /* Unlock the forks for other philosophers */ 
   printForks[LEFT_FORK] = FORK_DOWN;
   if (pthread_mutex_unlock(&(forks[LEFT_FORK])) != 0) {
      fprintf(stderr, "P thread error unlocking left fork \n");
      exit(-1);
   }
 
   printStatusLine();

   printForks[RIGHT_FORK] = FORK_DOWN;
   if (pthread_mutex_unlock(&(forks[RIGHT_FORK])) != 0) {
      fprintf(stderr, "P thread error unlocking right fork \n");
      exit(-1);
   }

   printStatusLine();
   
   /* Exit critical region */ 
   if (pthread_mutex_unlock(&putForkLock) != 0) {
      fprintf(stderr, "P thread error leaving put_forks \n");
      exit(-1);
   }
}

void takeLeftFork(int id) {
   /* Try to lock the left fork, if not - block */ 
   if (pthread_mutex_lock(&(forks[LEFT_FORK])) != 0) {
      fprintf(stderr, "P thread error locking left fork \n");
      exit(-1);
   }

   printForks[LEFT_FORK] = id;

   printStatusLine();
}

void takeRightFork(int id) {
   /* Try to lock the right fork, if not - block */ 
   if (pthread_mutex_lock(&(forks[RIGHT_FORK])) != 0) {
      fprintf(stderr, "P thread error locking right fork \n");
      exit(-1);
   }

   printForks[RIGHT_FORK] = id;

   printStatusLine();
}

void eat(int id) {
   state[id] = EATING;

   printStatusLine();

   /* Sleep for a random amount of time */ 
   dawdle();
}

void think(int id) {
   state[id] = THINKING;

   printStatusLine();

   /* Sleep for a random amount of time */ 
   dawdle();
}


void printHeader() {
   /* This will only work for 5 philosophers but thats okay 
    * for this implementation */ 

   printf("|=============|=============|=============|");
   printf("=============|=============|\n");

   printf("|      A      |      B      |      C      |");
   printf("      D      |      E      |\n");

   printf("|=============|=============|=============|");
   printf("=============|=============|\n");
}

void printFooter() {
   /* This will only work for 5 philosophers but thats okay 
    * for this implementation */ 

   printf("|=============|=============|=============|");
   printf("=============|=============|\n");
}

void printStatusLine() {
   /* Lock printing so we don't overwrite rows */ 
   if (pthread_mutex_lock(&printLock) != 0) {
      fprintf(stderr, "P thread error entering print function \n");
      exit(-1);
   }

   int i, j;

   printf("|");

   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      printf(" ");

      /* Printing the fork statuses. e.g.: "-12--" */ 
      for (j = 0; j < NUM_FORKS; j++) {
         if (printForks[j] == i)
            printf("%d", j);
         else 
            printf("-");
      }

      /* Printing the status of the philosopher */ 
      if (state[i] == CHANGING)
         printf("       |");
      else if (state[i] == EATING) 
         printf(" Eat   |");
      else 
         printf(" Think |");
   }

   printf("\n");

   /* Done printing line, unlock */ 
   if (pthread_mutex_unlock(&printLock) != 0) {
      fprintf(stderr, "P thread error leaving printing function \n");
      exit(-1);
   }
}

void initLocks() {
   int i; 

   if (pthread_mutex_init(&putForkLock, NULL) != 0) {
      fprintf(stderr, "Error initializing put_forks P Thread Mutex\n");
      exit(-1);
   }
   
   if (pthread_mutex_init(&takeForkLock, NULL) != 0) {
      fprintf(stderr, "Error initializing take_forks P Thread Mutex\n");
      exit(-1);
   }

   if (pthread_mutex_init(&printLock, NULL) != 0) {
      fprintf(stderr, "Error initializing print P Thread Mutex\n");
      exit(-1);
   }

   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      if (pthread_mutex_init(&(forks[i]), NULL) != 0) {
         fprintf(stderr, "Error initializing fork P Thread Mutex\n");
         exit(-1);
      }
   }
}

void destroyLocks() {
   int i;

   for (i = 0; i < NUM_PHILOSOPHERS; i++) { 
      if (pthread_mutex_destroy(&(forks[i])) != 0) {
         fprintf(stderr, "Error Destroying fork P Thread mutex \n");
         exit(-1);
      }
   }

   if (pthread_mutex_destroy(&putForkLock) != 0) {
      fprintf(stderr, "Error Destroying put fork P Thread mutex \n");
      exit(-1);
   }

   if (pthread_mutex_destroy(&takeForkLock) != 0) {
      fprintf(stderr, "Error Destroying get fork P Thread mutex \n");
      exit(-1);
   }

   if (pthread_mutex_destroy(&printLock) != 0) {
      fprintf(stderr, "Error Destroying print P Thread mutex \n");
      exit(-1);
   }
}

void dawdle() { 
/*
 * sleep for a random amount of time between 0 and 999
 * milliseconds. This routine is somewhat unreliable, since it
 * doesn’t take into account the possiblity that the nanosleep
 * could be interrupted for some legitimate reason. *
 * nanosleep() is part of the realtime library and must be linked
 * with –lrt */

   struct timespec tv;
   int msec = (int)(((double)random() / INT_MAX) * 1000);

   tv.tv_sec = 0;
   tv.tv_nsec = 1000000 * msec;
   if (-1 == nanosleep(&tv,NULL) ) {
      perror("nanosleep");
   }
}

int main(int argc, char *argv[]) {
   int i;
   int id[NUM_PHILOSOPHERS];
   pthread_t childid[NUM_PHILOSOPHERS];

   /* Seed random with time since epoch */ 
   srandom(time(0));

   /* Set the number of eat-think cycles */    
   if (argc > 1) {
      /* Non Default Cycles */ 
      numCycles = atoi(argv[1]);

      if (numCycles == 0) {
         /* Error with atoi or 0 */ 
         numCycles = DEFAULT_CYCLES;
      }
   }
   else {
      /* No command line argument, use default */ 
      numCycles = DEFAULT_CYCLES;
   }

   printHeader();

   initLocks();

   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      /* Initialize philosopher IDs */ 
      id[i] = i;	 

      /* Initialize printing array */ 
      printForks[i] = FORK_DOWN;

      /* Initialize philosopher states, they all start hungry */ 
      state[i] = CHANGING;
   }

   /* Spawn the philosophers */ 
   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      int res;
      res = pthread_create(
      	            &childid[i], 
                     NULL, 
                     philosopher, 
                     (void *) (&id[i]));

      if (res == -1) {
      	fprintf(stderr, "Error spawning child %i: %s\n", 
               i, strerror(errno));
      	exit(-1);
      }                       
   }

   /* Wait for all the philosophers */ 
   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      if (pthread_join(childid[i], NULL) != 0) {
         fprintf(stderr, "P thread error joining child \n");
         exit(-1);
      }
   }

   destroyLocks();

   printFooter();

   return 0;
}

















