#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void) {                 
    printf("before fork, parent pid: %d\n", getpid());
    printf("calling fork()\n");
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) 
    {                     
        printf("child procee: before close\n");
        close(STDOUT_FILENO);
        int ret = printf("child process: hello\n");
        if (ret < 0) {
            perror("printf failed");
        }
    } 
    else 
    {                            
        wait(NULL);
        printf("parent process: goodbye\n");
    }
    return 0;
}