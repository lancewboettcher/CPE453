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
#define DEFAULT_CYCLES 1
#define LEFT_FORK (id+1)%NUM_PHILOSOPHERS
#define RIGHT_FORK (id)
#define FORK_DOWN 100
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
void initPhilosophers(pthread_t *childid, int cycles);
void waitAndCleanupPhilosophers(pthread_t *childid);

int numCycles;                    /* Number of eat-think cycles to go through */
int state[NUM_PHILOSOPHERS];      /* EATING THINKING or CHANGING */ 
int printForks[NUM_FORKS];        /* index = fork #. value = phil # holding it*/
pthread_mutex_t forks[NUM_FORKS]; /* Locks on the forks */ 
pthread_mutex_t printLock;        /* Printing function lock */ 
pthread_mutex_t takeForkLock;     /* take_forks lock */ 
pthread_mutex_t putForkLock;      /* put_forks lock */ 

void *philosopher(void *id) {

   int myId = *(int *)id;
   int i; 

   /* Cycle through eating and thinking */ 
   for (i = 0; i < numCycles; i++) {
      take_forks(myId);
      eat(myId);
      put_forks(myId);
      think(myId);

      state[myId] = CHANGING;
   }

   return NULL;
}

void take_forks(int id) {
   /* Enter critical region */ 
   pthread_mutex_lock(&takeForkLock);
  
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

   /* Exit critical region */ 
   pthread_mutex_unlock(&takeForkLock);
}

void put_forks(int id) {
   /* Enter Critical Region */ 
   pthread_mutex_lock(&putForkLock);
   
   /* Philosopher wants to think */ 
   state[id] = CHANGING;

   printStatusLine();

   /* Unlock the forks for other philosophers */ 
   printForks[LEFT_FORK] = FORK_DOWN;
   pthread_mutex_unlock(&(forks[LEFT_FORK]));
 
   printStatusLine();

   printForks[RIGHT_FORK] = FORK_DOWN;
   pthread_mutex_unlock(&(forks[RIGHT_FORK]));

   printStatusLine();
   
   /* Exit critical region */ 
   pthread_mutex_unlock(&putForkLock);
}

void takeLeftFork(int id) {
   /* Try to take the fork. We will block here if another philosopher
    * is using it */ 
   pthread_mutex_lock(&(forks[LEFT_FORK]));
   printForks[LEFT_FORK] = id;

   printStatusLine();
}

void takeRightFork(int id) {
   /* Try to take the fork. We will block here if another philosopher
    * is using it */ 
   pthread_mutex_lock(&(forks[LEFT_FORK]));
   pthread_mutex_lock(&(forks[RIGHT_FORK]));
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
   printf("|=============|=============|=============|");
   printf("=============|=============|\n");

   printf("|      A      |      B      |      C      |");
   printf("      D      |      E      |\n");

   printf("|=============|=============|=============|");
   printf("=============|=============|\n");
}

void printFooter() {
   printf("|=============|=============|=============|");
   printf("=============|=============|\n");
}

void printStatusLine() {
   /* Lock printing so we don't overwrite rows */ 
   pthread_mutex_lock(&printLock);

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
   pthread_mutex_unlock(&printLock);
}

void initPhilosophers(pthread_t *childid, int cycles) {
   int i;
   int id[NUM_PHILOSOPHERS];

   printHeader();

   /* Seed random with time since epoch */ 
   srandom(time(0));

   numCycles = cycles; 

   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      /* Initialize philosopher IDs */ 
      id[i] = i;	 

      /* Initialize locks */ 
      if (pthread_mutex_init(&(forks[i]), NULL) != 0) {
         fprintf(stderr, "Error initializing mutex \n");
         exit(-1);
      }

      /* Initialize printing array */ 
      printForks[i] = FORK_DOWN;

      /* Initialize philosopher states */ 
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
      	fprintf(stderr, "Child %i: %s\n", i, strerror(errno));
      	exit(-1);
      }                       
   }
}

void waitAndCleanupPhilosophers(pthread_t *childid) {
   int i;

   /* Wait for all the philosophers */ 
   for (i = 0; i < NUM_PHILOSOPHERS; i++) {
      pthread_join(childid[i], NULL);
   }

   /* Destroy the locks */
   for (i = 0; i < NUM_PHILOSOPHERS; i++) { 
      pthread_mutex_destroy(&(forks[i]));
   }

   printFooter();
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
   pthread_t childid[NUM_PHILOSOPHERS];

   int cycles;

   /* Get the number of eat-think cycles */    
   if (argc > 1) {
      /* Non Default Cycles */ 
      cycles = atoi(argv[1]);
   }
   else {
      /* No command line argument, use default */ 
      cycles = DEFAULT_CYCLES;
   }

   initPhilosophers(childid, cycles);
   waitAndCleanupPhilosophers(childid);

   return 0;
}

















