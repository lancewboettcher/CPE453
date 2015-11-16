#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/ioc_secret.h>

int main(int argc, char *argv[]) {
    int fd, res, uid; 
    char msg[100];
    
    msg[0] = 'h';
    msg[1] = 'i'; 

    fd = open("/dev/Secret", 2);

    printf("Opening... fd=%d\n", fd);

    res = write(fd, msg, strlen(msg));

    printf("Writing... res=%d\n", res);

    if (argc > 1 && 0 != (uid=atoi(argv[1]))) {
        if (res = ioctl(fd, SSGRANT, &uid) ) {
            perror("ioctl");
        }
        printf("Trying to change owner to %d ... res=%d\n", uid, res);
    }

    res = close(fd);

    return 0;
}    
