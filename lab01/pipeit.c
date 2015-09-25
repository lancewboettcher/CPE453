#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {

   int lsPid, sortPid, fds[2], outputFd;
   int lsReturnValue, sortReturnValue, lsReturnStatus, sortReturnStatus; 

   pipe(fds);

   if ((lsPid = fork())) {  //ls Parent

      if ((sortPid = fork())) { //sort Parent

         close(fds[0]);
         close(fds[1]);

         lsReturnValue = waitpid(lsPid, &lsReturnStatus, 0);

         sortReturnValue = waitpid(sortPid, &sortReturnStatus, 0);

         if (WEXITSTATUS(lsReturnStatus) > 0) {
            fprintf(stderr, "ls returned error with exit Code: %d\n", 
                WEXITSTATUS(lsReturnStatus));
               
            return -1;
         }

         if (WEXITSTATUS(sortReturnStatus) > 0) {
            fprintf(stderr, "sort returned error with exit Code: %d\n",
                WEXITSTATUS(sortReturnStatus));
               
            return -1;
         }
      }

      else { //sort Child
         dup2(fds[0], STDIN_FILENO); 

         close(fds[0]);
         close(fds[1]);
        
         outputFd = open("outfile", O_RDWR | O_CREAT | O_TRUNC);
         dup2(outputFd, STDOUT_FILENO);

         execlp("sort", "sort", "-r", NULL);
         printf("sort exec failed\n");
      }

   }
   else {   //ls Child
      dup2(fds[1], STDOUT_FILENO); 

      close(fds[0]);
      close(fds[1]);

      execl("/bin/ls", "ls", NULL);
      printf("ls exec failed\n");
   }   

   return 0;
}
