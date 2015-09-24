#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {

   int lsPid, sortPid, fds[2], lsReturnValue, sortReturnValue, lsReturnStatus, sortReturnStatus; 

   pipe(fds);

   if ((lsPid = fork())) {  //ls Parent

      if ((sortPid = fork())) { //sort Parent

         close(fds[0]);
         close(fds[1]);

         lsReturnValue = waitpid(lsPid, &lsReturnStatus, 0);

         printf("done waiting for ls\n");

         sortReturnValue = waitpid(sortPid, &sortReturnStatus, 0);

         printf("done waiting for sort\n");

         if (WEXITSTATUS(lsReturnStatus) > 0) {
            fprintf(stderr, "ls returned error with exit Code: %d\n", WEXITSTATUS(lsReturnStatus));
               
            return -1;
         }

         if (WEXITSTATUS(sortReturnStatus) > 0) {
            fprintf(stderr, "sort returned error with exit Code: %d\n", WEXITSTATUS(sortReturnStatus));
               
            return -1;
         }
      }

      else { //sort Child
         dup2(fds[1], STDOUT_FILENO); 
         close(fds[0]);
         close(fds[1]);

         execl("/bin/ls", "ls", (char *)0);
         printf("ls exec failed\n");
      }

   }
   else {   //ls Child
      dup2(fds[0], STDIN_FILENO); 
      close(fds[0]);
      close(fds[1]);

      execlp("sort", "-r", (char *)0);
      printf("sort exec failed\n");
   }   

   return 0;
}
