#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    int x = 100;                        
    printf("before fork, parent pid: %d, x=%d\n", getpid(), x);
    printf("calling fork()\n");
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) 
    {                     
        printf("after fork, child pid: %d, x before change=%d\n", getpid(), x);

        x += 50;
        printf("after fork, child pid: %d, x after change=%d\n", getpid(), x);
    } 
    else 
    {                            
        printf("after fork, parent pid: %d, x before change=%d\n", getpid(), x);

        x += 10;                        
        printf("after fork, parent pid: %d, x after change=%d\n", getpid(), x);
    }
    return 0;
}
