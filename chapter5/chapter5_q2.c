#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void) {
    int fd = open("test.txt", O_CREAT | O_WRONLY | O_TRUNC);                   
    printf("before fork, parent pid: %d, fd=%d\n", getpid(), fd);
    printf("calling fork()\n");
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) 
    {                     
        const char* msg = "Hello child process\n";
        write(fd, msg, strlen(msg));
    } 
    else 
    {                            
        const char* msg = "Hello parent process\n";
        write(fd, msg, strlen(msg));
    }
    return 0;
}
